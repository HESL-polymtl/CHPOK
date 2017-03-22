#******************************************************************
#
# Institute for System Programming of the Russian Academy of Sciences
# Copyright (C) 2016 ISPRAS
#
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, Version 3.
#
# This program is distributed in the hope # that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU General Public License version 3 for more details.
#
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

"""
Defines class 'Configuration' containing configuration of the whole module.
"""

from __future__ import print_function

import abc
import functools
import collections
import ipaddr
import math

# Single time slot for execute something.
#
# - duration - duration of given slot, in miliseconds.
class TimeSlot():
    __metaclass__ = abc.ABCMeta
    __slots__ = [
        "duration",

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    @abc.abstractmethod
    def get_kind_constant(self):
        pass

    def __init__(self, duration):
        if duration < (10 ** 6):
            raise ValueError("Minimum value for Slot duration is 1000000 (1ms).")

        self.duration = duration

        self.private_data = None

    def validate(self):
        if not isinstance(self.duration, int):
            raise TypeError

class TimeSlotSpare(TimeSlot):
    __slots__ = []

    def get_kind_constant(self):
        return "POK_SLOT_SPARE"

    def __init__(self, duration):
        TimeSlot.__init__(self, duration)

# Time slot used for execute partition.
#
# - partition - reference to the partition to execute during given timeslot.
# - periodic_processing_start - whether given slot can be used for start periodic processes.
class TimeSlotPartition(TimeSlot):
    __slots__ = [
        "partition",
        "periodic_processing_start",
    ]

    def get_kind_constant(self):
        return "POK_SLOT_PARTITION"

    def __init__(self, duration, partition, periodic_processing_start):
        TimeSlot.__init__(self, duration)
        self.partition = partition
        self.periodic_processing_start = periodic_processing_start

    def validate(self):
        super(TimeSlotPartition, self).validate()

        if not isinstance(self.periodic_processing_start, bool):
            raise TypeError

# Time slot for monitor.
class TimeSlotMonitor(TimeSlot):
    __slots__ = []

    def __init__(self, duration):
        TimeSlot.__init__(self, duration)

    def get_kind_constant(self):
        return "POK_SLOT_MONITOR"
class TimeSlotGDB(TimeSlot):
    __slots__ = []
    def __init__(self, duration):
        TimeSlot.__init__(self, duration)
    def get_kind_constant(self):
        return "POK_SLOT_GDB"


class HMAction:
    """
    Generic action in Health Monitor tables.

    Subclasses should define 'possible_level' and 'possible_recovery_action'
    lists with possible values for corresponded fields.
    """
    __slots__ = [
        'level', # On which level error should be proceed.
        'recovery_action' # Recovery action as a string.
    ]

    def __init__(self, level, recovery_action):
        if level not in self.__class__.possible_level:
            raise RuntimeError("Level value '%s' is not suitable for HMAction object of type '%s'" % (level, str(self.__class__)))
        if recovery_action not in self.__class__.possible_recovery_action:
            raise RuntimeError("Recovery action value '%s' is not suitable for HMAction object of type '%s'" % (recovery_action, str(self.__class__)))

        self.level = level
        self.recovery_action = recovery_action

class HMTable:
    """
    Generic Health Monitor table.
    """
    # Possible system states(ordered, without prefix)
    system_states = [
        'INIT_PARTOS',
        'INIT_PARTUSER',
        'INTERRUPT_HANDLER',
        'OS_MOD',
        'OS_PART',
        'ERROR_HANDLER',
        'USER'
    ]

    # Possible error identificators(ordered, without prefix)
    error_ids = [
        'MODPOSTPROCEVENT_ELIST',
        'ILLEGAL_REQUEST',
        'APPLICATION_ERROR',
        'PARTLOAD_ERROR',
        'NUMERIC_ERROR',
        'MEMORY_VIOLATION',
        'DEADLINE_MISSED',
        'HARDWARE_FAULT',
        'POWER_FAIL',
        'STACK_OVERFLOW',
        'PROCINIT_ERROR',
        'NOMEMORY_PROCDATA',
        'ASSERT',
        'CONFIG_ERROR',
        'CHECK_POOL',
        'UNHANDLED_INT'
    ]


    __slots__ = [
        'actions', # Mapping from (system_state) * (error_id) to HMAction object.
        'default_action' # Action object for use if concrete mapping is absent.
    ]

    def __init__(self):
        self.actions = {}
        # Do not define default_action.

    def get_action(self, system_state, error_id):
        actions_per_state = self.actions.get(system_state, {})
        # Reference self.default_action only when action is REALLY absent.
        action = actions_per_state.get(error_id, None)
        if action is None:
            action = self.default_action

        return action


class ModuleHMAction(HMAction):
    """
    Module Health Monitor action.
    """
    possible_level = ['MODULE', 'PARTITION']
    possible_recovery_action = ['IGNORE', 'SHUTDOWN', 'RESET']

class ModuleHMTable(HMTable):
    """
    Module Health Monitor Table.
    """
    pass


class PartitionHMAction(HMAction):
    """
    Partition Health Monitor action.

    Additional slots are for PROCESS-level errors in 'USER' state.
    """
    possible_level = ['PARTITION', 'PROCESS']
    possible_recovery_action = ['IGNORE', 'IDLE', 'COLD_START', 'WARM_START']

    __slots__ = [
        'error_code',
        'description'
    ]

    def __init__(self, level, recovery_action, error_code = None, description = None):
        HMAction.__init__(self, level, recovery_action)
        if error_code is not None:
            self.error_code = error_code
            self.description = description

class PartitionHMTable(HMTable):
    """
    Partition Health Monitor table.
    """
    # List of system states corresponded to partition.
    # Mapping exists only for those states.
    partition_system_states = [
        'OS_PART',
        'ERROR_HANDLER',
        'USER'
    ]

# Memory block, belonging to partition's address space.
# Actually, this is a **requirement** to a memory block, not a
# memory block's full definition.
class MemoryBlock:

    __slots__ = [
        "name", # Name of the block. Should be unique in partition.
        "size", # Size of the block. Should be non-negative.
        "align", # Alignment of the block. 4k by default.
        # Cache policy for given memory block. One of:
        #
        # - OFF
        # - COPY_BACK
        # - WRITE_THRU
        # - OFF+COHERENCY
        # - COPY_BACK+COHERENCY
        # - WRITE_THRU+COHERENCY
        # - OFF+GUARDED
        # - COPY_BACK+GUARDED
        # - WRITE_THRU+GUARDED
        # - OFF+GUARDED+COHERENCY
        # - COPY_BACK+GUARDED+COHERENCY
        # - WRITE_THRU+GUARDED+COHERENCY
        # - IO
        # - DEFAULT
        #
        # By default, it is "DEFAULT"
        "cache_policy",
        "access", # Access to the memory block from the partition. String contained of 'R', 'W', 'X'.
        "is_contiguous", # Whether block should be *physically* contiguous.
        "vaddr", # Virtual address of the block, if required.
        "paddr", # Physical address of the block, if required. Implies 'is_contiguous' to be True.

        # Source, from which memory block should be initialized.
        # Possible values see in 'init_source_values' list below.
        #
        # If memory block doesn't required initialization, leave this as None.
        "init_source",

        # Stage, when memory block should be initialized:
        # Possible values see in 'init_stage_values' list below.
        #
        # If 'init_source' is None this should be None too.
        "init_stage",

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    # Possible values for memory block's 'init_source' attribute.
    init_source_values = [
        'ZERO', #fill with zeroes.
        # (Value 'ELF' is internal for implementation).
    ]

    # Possible values for memory block's 'init_stage' attribute.
    init_stage_values = [
        'MODULE', # Every time when module is started or restarted
        'PARTITION', # Every time partition is started (either in COLD_START or in WARM_START mode).
        'PARTITION:COLD', # Every time when partition is started in COLD_START mode.
    ]


    def __init__(self, name, size):
        self.name = name
        self.size = size

        self.vaddr = None # Non-specified by default
        self.paddr = None # Non-specified by default

        self.cache_policy = "DEFAULT"

        self.is_coherent = False
        self.is_guarded = False

        self.is_writable = True # RW by default

        self.is_contiguous = False # By default, physical contiguous not required.

        self.align = 0x1000 # By default, alignment 4K.

        self.init_source = None
        self.init_stage = None


# ARINC partition.
#
# - name - name of the partition
# ...
# - part_id - identificator(number) of given partition.
# - part_index - index of the partition in the array. Filled automatically.
class Partition:
    __slots__ = [
        "name",

        "is_system",

        # Memory required for code and data defined in partition's elf.
        #
        # Such parameter implies that *whole* elf can be mapped into
        # *single memory block*, which applies too strong constraint
        # on elf layout.
        #
        # 'BOARD_DEPLOY' script may ignore this parameter and extract
        # layout information directly from the partition's elf file.
        #
        # Will be removed in the future.
        "memory_size",

        # Requested size of the heap.
        #
        # Note: ARINC requirements for buffers and co. shouldn't be counted here.
        "heap_size",

        "num_threads", # number of user threads, _not_ counting init thread and error handler

        'stack_size_all', # Total memory for all stacks

        "ports_queueing", # list of queuing ports
        "ports_sampling", # list of sampling ports

        "num_arinc653_buffers",
        "num_arinc653_blackboards",
        "num_arinc653_semaphores",
        "num_arinc653_events",

        "buffer_data_size", # bytes allocated for buffer data
        "blackboard_data_size", # same, for blackboards
    
        "logbooks_data",    # list of logbooks

        "hm_table", # partition hm table

        "memory_blocks", # List of memory blocks

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    def __init__(self, part_id, name):
        self.name = name
        self.part_id = part_id

        self.memory_size = 0

        # If application needs, it may set 'period' and 'duration' attributes.
        #
        # Otherwise, these attributes are treated as:
        #
        # 'period' is major time frame
        # 'duration' is sum of all timeslots, denoted for partition.
        self.period = None
        self.duration = None

        self.heap_size = 0

        self.num_threads = 0

        self.stack_size_all = None # Should be assigned later explicitely.

        self.num_arinc653_buffers = 0
        self.num_arinc653_blackboards = 0
        self.num_arinc653_semaphores = 0
        self.num_arinc653_events = 0

        self.buffer_data_size = 0
        self.blackboard_data_size = 0
        
        self.logbooks_data = []

        self.hm_table = PartitionHMTable()

        self.ports_queueing = []
        self.ports_sampling = []

        self.memory_blocks = []


def _get_port_direction(port):
    direction = port.direction.lower()
    if direction in ("source", "out"):
        return "POK_PORT_DIRECTION_OUT"
    if direction in ("destination", "in"):
        return "POK_PORT_DIRECTION_IN"
    raise ValueError("%r is not valid port direction" % port["direction"])


# Common parameters for Sampling and Queueing ports.
class Port:
    __metaclass__ = abc.ABCMeta
    __slots__ = [
        "name",
        "is_direction_src",
        "max_message_size",

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    @abc.abstractmethod
    def get_kind_constant(self):
        pass

    def __init__(self, name, direction, max_message_size):
        self.name = name
        _direction = direction.lower()
        if _direction in ("source", "out"):
            self.is_direction_src = True
        elif _direction in ("destination", "in"):
            self.is_direction_src = False
        else:
            raise ValueError("%r is not valid port direction" % direction)

        self.max_message_size = max_message_size

        self.private_data = None

    def is_src(self):
        return self.is_direction_src;

    def is_dst(self):
        return not self.is_direction_src;


class SamplingPort(Port):
    __slots__ = [
        "refresh",
    ]

    def get_kind_constant(self):
        return "sampling"

    def __init__(self, name, direction, max_message_size, refresh):
        Port.__init__(self, name, direction, max_message_size)
        if refresh < (10 ** 6):
                raise ValueError("Minimum value for refresh is 1000000 (1ms).")

        self.refresh = refresh

class QueueingPort(Port):
    __slots__ = [
        "max_nb_message",
    ]

    def get_kind_constant(self):
        return "queueing"

    def __init__(self, name, direction, max_message_size, max_nb_message):
        Port.__init__(self, name, direction, max_message_size)
        self.max_nb_message = max_nb_message


# Generic channel connecting two connections.
class Channel:
    __metaclass__ = abc.ABCMeta
    __slots__ = [
        # Source and destination sides (of type 'Connection') of the channel
        "src",
        "dst",
        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    def __init__(self, src, dst):
        self.src = src
        self.dst = dst

        self.private_data = None

    def validate(self):
        if not isinstance(self.src, Connection):
            raise TypeError
        if not isinstance(self.dst, Connection):
            raise TypeError

        if self.get_local_connection() == None:
            raise ValueError("at least one connection per channel must be local")

        self.src.validate()
        self.dst.validate()

# One point of the channel
class Connection():
    __metaclass__ = abc.ABCMeta
    @abc.abstractmethod
    def get_kind_constant(self):
        pass

# Connection to partition's port.
class LocalConnection(Connection):
    __slots__ = [
        "partition",
        "port",

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    def __init__(self, partition, port):
        self.partition = partition
        self.port = port

        self.private_data = None

    def get_kind_constant(self):
        return "Local"


class MemoryBlockRef:
    """
    Reference to the memory block from sharing.
    """
    __slots__ = [
        'partition', # Partition to which memory block belongs to.
        'memory_block', # Memory block itself
    ]

    def __init__(self, partition, memory_block):
        self.partition = partition
        self.memory_block = memory_block


class MemoryBlockSharing:
    """
    Memory blocks which should be mapped into the same physical memory.
    """
    __slots__ = [
        'mb_refs' # List of MemoryBlockRef objects.
    ]

    def __init__(self):
        self.mb_refs = []

class Configuration:
    __slots__ = [
        "module_hm_table", # Health Monitor Table assosiated with the module
        "partitions", # List of partitions
        "slots", # time windows
        "channels", # List of channels

        # if this is set, POK writes a special string once
        # there are no more schedulable threads
        # it's used by test runner as a sign that POK
        # can be terminated
        "test_support_print_when_all_threads_stopped",

        "major_frame", # May be set manually for self-check

        "memory_block_sharings", # List of MemoryBlockSharing objects.

        # Data private for implementation.
        #
        # During filling the object, you may use this field as you want.
        "private_data"
    ]

    def __init__(self):
        self.module_hm_table = ModuleHMTable()

        self.partitions = []
        self.slots = []
        self.channels = []

        self.test_support_print_when_all_threads_stopped = False

        self.major_frame = None # Not set yet.

        self.memory_block_sharings = []

        self.private_data = None
