#  Copyright (C) 2014 Maxim Malkov, ISPRAS <malkov@ispras.ru> 
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
This file defines classes and functions that can be used to
simplify generation of POK kernel configuration.

"""

import sys
import os
import abc
import json
import functools
import collections
import ipaddress

class TimeSlot(metaclass=abc.ABCMeta):
    __slots__ = ["duration"]

    @abc.abstractmethod
    def get_kind_constant(self):
        pass

    def validate(self):
        if not isinstance(self.duration, int):
            raise TypeError
        
class TimeSlotSpare(TimeSlot):
    __slots__ = []

    def get_kind_constant(self):
        return "POK_SLOT_SPARE"

class TimeSlotPartition(TimeSlot):
    __slots__ = [
        "partition",
        "periodic_processing_start",
    ]

    def get_kind_constant(self):
        return "POK_SLOT_PARTITION"

    def validate(self):
        super().validate()

        if not isinstance(self.periodic_processing_start, bool):
            raise TypeError

        if not isinstance(self.partition, int):
            raise TypeError

class TimeSlotNetwork(TimeSlot):
    __slots__ = []

    def get_kind_constant(self):
        return "POK_SLOT_NETWORKING"

class Partition:
    __slots__ = [
        "name", 

        "size", # allocated RAM size in bytes (code + static storage)
        "num_threads", # number of user threads, _not_ counting init thread and error handler
        "ports", # list of ports

        "num_arinc653_buffers",
        "num_arinc653_blackboards",
        "num_arinc653_semaphores",
        "num_arinc653_events",

        "buffer_data_size", # bytes allocated for buffer data
        "blackboard_data_size", # same, for blackboards

        "hm_table", # partition hm table
    ]

    def get_all_ports(self):
        return list(self.ports)

    def get_all_sampling_ports(self):
        return [port for port in self.ports if isinstance(port, SamplingPort)]

    def get_all_queueing_ports(self):
        return [port for port in self.ports if isinstance(port, QueueingPort)]

    def validate(self):
        # validation is by no means complete
        # it's just basic sanity check

        for attr in self.__slots__:
            if not hasattr(self, attr):
                raise ValueError("%r is not set for %r" % (attr, self))

        for port in self.ports:
            port.validate()

    def get_needed_lock_objects(self):
        # all of them implicitly require a lock object
        return (
            len(self.ports) + 
            self.num_arinc653_buffers +
            self.num_arinc653_blackboards +
            self.num_arinc653_semaphores +
            self.num_arinc653_events
        )

    def get_needed_threads(self):
        return (
            1 + # init thread
            1 + # error handler
            self.num_threads
        )

    def get_port_by_name(self, name):
        res = next((port for port in self.get_all_ports() if port.name == name), None)
        if not res:
            raise ValueError("no such named port %r in %r" % (name, self))
        return res

def _get_port_direction(port):
    direction = port.direction.lower()
    if direction in ("source", "out"):
        return "POK_PORT_DIRECTION_OUT"
    if direction in ("destination", "in"):
        return "POK_PORT_DIRECTION_IN"
    raise ValueError("%r is not valid port direction" % port["direction"])


class SamplingPort:
    __slots__ = [
        "name",
        "direction",
        "max_message_size",
        "refresh",
    ]

    def validate(self):
        for attr in self.__slots__:
            if not hasattr(self, attr):
                raise ValueError("%r is not set for %r" % (attr, self))

class QueueingPort:
    __slots__ = [
        "name",
        "direction",
        "max_nb_messages",
        "max_message_size",
    ]

    def validate(self):
        for attr in self.__slots__:
            if not hasattr(self, attr):
                raise ValueError("%r is not set for %r" % (attr, self))

class Channel:
    __slots__ = ["src", "dst"]

    def __init__(self, src=None, dst=None):
        self.src = src
        self.dst = dst

    def validate(self):
        if not isinstance(self.src, Connection):
            raise TypeError
        if not isinstance(self.dst, Connection):
            raise TypeError

        if self.get_local_connection() == None:
            raise ValueError("at least one connection per channel must be local")

        self.src.validate()
        self.dst.validate()

    def get_local_connection(self):
        if isinstance(self.src, LocalConnection):
            return self.src
        if isinstance(self.dst, LocalConnection):
            return self.dst
        return None

    def is_queueing(self):
        return isinstance(self.get_local_connection().port, QueueingPort)
    
    def is_sampling(self):
        return isinstance(self.get_local_connection().port, SamplingPort)

    def requires_network(self):
        return any(isinstance(x, UDPConnection) for x in [self.src, self.dst])

class Connection(metaclass=abc.ABCMeta):
    @abc.abstractmethod
    def validate(self):
        pass

class LocalConnection(Connection):
    __slots__ = ["port"]

    def validate(self):
        if not hasattr(self, "port") or self.port == None:
            raise ValueError

        if not isinstance(self.port, (QueueingPort, SamplingPort)):
            raise TypeError

class UDPConnection(Connection):
    __slots__ = ["host", "port"]

    def validate(self):
        if not hasattr(self, "host"):
            raise AttributeError("host")

        if not isinstance(self.host, ipaddress.IPv4Address):
            raise TypeError(type(self.host))

        if not hasattr(self, "port"):
            raise AttributeError("port")

        if not isinstance(self.port, int):
            raise TypeError(type(self.port))

        if port < 0 or port > 0xFFFF:
            raise ValueError(self.port)

class NetworkConfiguration:
    __slots__ = [
        #"mac", # mac address
        "ip", # IP used as source
    ]

    def validate(self):
        #if not hasattr(self, "mac"):
        #    raise AttributeError
        #if self.mac is not None:
        #    if not isinstance(self.mac, bytes):
        #        raise TypeError
        #    if len(self.mac) != 6:
        #        raise ValueError
        #    if not (self.mac[0] & 0x2):
        #        print("Warning! MAC address is not locally administered one", file=sys.stderr)
        
        if not hasattr(self, "ip"):
            raise AttributeError
        if not isinstance(self.ip, ipaddress.IPv4Address):
            raise TypeError

    #def mac_to_string(self):
    #    return "{%s}" % ", ".join(hex(i) for i in self.mac)

class Configuration:

    __slots__ = [
        "partitions", 
        "slots", # time windows
        "channels", # queueing and sampling port channels (connections)
        "network", # NetworkConfiguration object (or None)

        # if this is set, POK writes a special string once 
        # there are no more schedulable threads
        # it's used by test runner as a sign that POK
        # can be terminated
        "test_support_print_when_all_threads_stopped",  
    ]

    def __init__(self):
        self.partitions = []
        self.slots = []
        self.channels = []
        self.network = None

        self.test_support_print_when_all_threads_stopped = False

    def get_all_ports(self):
        return sum((part.get_all_ports() for part in self.partitions), [])

    def get_all_sampling_ports(self):
        return sum((part.get_all_sampling_ports() for part in self.partitions), [])

    def get_all_queueing_ports(self):
        return sum((part.get_all_queueing_ports() for part in self.partitions), [])

    def get_partition_by_name(self, name):
        return [
            part
            for part in self.partitions
            if part.name == name
        ][0]

    def get_port_by_partition_and_name(self, partition_name, port_name):
        return self.get_partition_by_name(partition_name).get_port_by_name(port_name)

    def validate(self):
        for part in self.partitions:
            part.validate()

        # network stuff
        networking_time_slot_exists = any(isinstance(slot, TimeSlotNetwork) for slot in self.slots)

        if self.network:
            self.network.validate()

            if not networking_time_slot_exists: 
                raise ValueError("Networking is enabled, but no dedicated network processing time slot is present") 
        else:
            if networking_time_slot_exists:
                raise ValueError("Networking is disabled, but there's (unnecessary) network processing time slot in the schedule")

            if any(chan.requires_network() for chan in self.channels):
                raise ValueError("Network channel is present, but networking is not configured")
            
        # validate schedule
        partitions_set = set(range(len(self.partitions)))
        partitions_without_periodic_processing = set(partitions_set) # copy

        for slot in self.slots:
            slot.validate()

            if isinstance(slot, TimeSlotPartition):
                if slot.partition >= len(self.partitions):
                    raise ValueError("slot doesn't correspond to existing partition")
                
                if slot.periodic_processing_start:
                    partitions_without_periodic_processing.discard(slot.partition)


        if partitions_without_periodic_processing:
            raise ValueError("partitions %r don't have periodic processing points set" % partitions_without_periodic_processing)
        

    def get_all_channels(self):
        return self.channels

COMMON_KERNEL_DEFINES = """\
#define POK_NEEDS_LOCKOBJECTS  1
#define POK_NEEDS_THREADS      1
#define POK_NEEDS_PARTITIONS   1
#define POK_NEEDS_SCHED        1
#define POK_NEEDS_TIME         1
#define POK_NEEDS_GETTICK      1
#define POK_NEEDS_DEBUG        1
#define POK_NEEDS_SERIAL       1
#define POK_NEEDS_CONSOLE      1
#define POK_NEEDS_ERROR_HANDLING 1
#define POK_NEEDS_THREAD_SUSPEND 1
#define POK_NEEDS_THREAD_SLEEP 1
#define POK_NEEDS_THREAD_ID 1
""" 

COMMON_PARTITION_DEFINES = """\
#define POK_NEEDS_TIMER 1
#define POK_NEEDS_THREADS 1

#define POK_NEEDS_PARTITIONS 1

#define POK_NEEDS_DEBUG 1
#define POK_NEEDS_CONSOLE 1

#define POK_NEEDS_LIBC_STDLIB 1
#define POK_CONFIG_NEEDS_FUNC_MEMCPY         1
#define POK_CONFIG_NEEDS_FUNC_MEMSET         1
#define POK_CONFIG_NEEDS_FUNC_MEMCMP         1
#define POK_CONFIG_NEEDS_FUNC_STRCMP         1
#define POK_CONFIG_NEEDS_FUNC_STRNCMP        1
#define POK_CONFIG_NEEDS_FUNC_STRCPY         1
#define POK_CONFIG_NEEDS_FUNC_STRNCPY        1
#define POK_CONFIG_NEEDS_FUNC_STRLEN         1
#define POK_CONFIG_NEEDS_FUNC_STREQ          1
#define POK_CONFIG_NEEDS_FUNC_ITOA           1
#define POK_CONFIG_NEEDS_FUNC_UDIVDI3        1

#define POK_NEEDS_MIDDLEWARE 1

#define POK_NEEDS_ARINC653_PARTITION 1
#define POK_NEEDS_ARINC653_PROCESS 1
#define POK_NEEDS_ARINC653_ERROR 1
#define POK_NEEDS_ARINC653_SAMPLING 1
#define POK_NEEDS_ARINC653_QUEUEING 1
#define POK_NEEDS_ARINC653_TIME 1
"""

TIMESLOT_SPARE_TEMPLATE = """\
    { .type = POK_SLOT_SPARE,
      .duration = %(duration)d,
    },
"""

TIMESLOT_PARTITION_TEMPLATE = """\
    { .type = POK_SLOT_PARTITION,
      .duration = %(duration)d,
      .partition = 
      { .id = %(partition)d,
        .periodic_processing_start = %(periodic_processing_start)s,
      }
    },
"""

TIMESLOT_NETWORKING_TEMPLATE = """\
    { .type = POK_SLOT_NETWORKING,
      .duration = %(duration)d,
    },
"""

SAMPLING_PORT_TEMPLATE = """\
    {
        .header = {
            .kind = POK_PORT_KIND_SAMPLING,
            .name = %(name)s,
            .partition = %(partition)s,
            .direction = %(direction)s,
        },
        .max_message_size = %(max_message_size)s,
        .data = (void *) %(data)s,
    },
"""

SAMPLING_PORT_DATA_TEMPLATE = """\
static struct {
    pok_port_size_t message_size;
    char data[%(max_message_size)d];
} %(varname)s;
"""

QUEUEING_PORT_TEMPLATE = """\
    {
        .header = {
            .kind = POK_PORT_KIND_QUEUEING,
            .name = %(name)s,
            .partition = %(partition)s,
            .direction = %(direction)s,
        },
        .max_message_size = %(max_message_size)d,
        .max_nb_messages = %(max_nb_messages)d,
        
        .data = (void *) %(data)s,
        .data_stride = %(data_stride)s,
    },
"""

QUEUEING_PORT_DATA_TEMPLATE = """\
static struct {
    pok_port_size_t message_size;
    char data[%(max_message_size)d];
} %(varname)s[%(max_nb_messages)d];
"""

PORT_CONNECTION_NULL = """\
    { .kind = POK_PORT_CONNECTION_NULL
    }
"""

PORT_CONNECTION_LOCAL_TEMPLATE = """\
    { .kind = POK_PORT_CONNECTION_LOCAL, 
      .local =  {
        .port_id = %(port_id)d, 
      }
    }
"""

PORT_CONNECTION_UDP_TEMPLATE = """
    { .kind = POK_PORT_CONNECTION_UDP,
      .udp = {.ptr = %s }
    }
"""

# this one is terrible
#
# we're allocating structure for both
# misc. info (like ip, port, and status flag),
# and for send buffer
#
# the latter is "variable" (it's static, but depends on the port),
# so we use this union trick to allocate extra memory
# after the end of structure
SAMPLING_PORT_UDP_SEND_BUFFER_TEMPLATE = """
static union {
    char foo[
        sizeof(pok_port_connection_sampling_udp_send_t) +
        // message buffer starts here
        POK_NETWORK_OVERHEAD +
        %(max_message_size)d // max message size
    ];
    pok_port_connection_sampling_udp_send_t info;
} %(varname)s = {
    .info = {
        .ip = %(ip)s,
        .port = %(port)d,
        .buffer_being_used = FALSE,
    },
};
"""

SAMPLING_PORT_UDP_RECV_BUFFER_TEMPLATE = """
static struct {
    pok_port_connection_sampling_udp_recv_t info;
} %(varname)s = {
    .info = {
        .ip = %(ip)s,
        .port = %(port)d,
    },
};
"""

QUEUEING_PORT_UDP_SEND_BUFFER_TEMPLATE = """
static union {
    char foo[
        sizeof(pok_port_connection_queueing_udp_send_t) +
        sizeof(
            pok_port_connection_queueing_udp_send_aux_t 
        ) * %(max_nb_messages)d
    ];

    pok_port_connection_queueing_udp_send_t info;
} %(varname)s = {
    .info = {
        .ip = %(ip)s,
        .port = %(port)d,
    },
};
"""

QUEUEING_PORT_UDP_RECV_BUFFER_TEMPLATE = """
static struct {
    pok_port_connection_queueing_udp_recv_t info;
} %(varname)s = {
    .info = {
        .ip = %(ip)s,
        .port = %(port)d,
    },
};
"""

def _c_string(b):
    # accidentally, JSON string is very close to C string literal
    return json.dumps(b)

def write_configuration(conf, kernel_dir, partition_dirs):
    with open(os.path.join(kernel_dir, "deployment.h"), "w") as f:
        write_kernel_deployment_h(conf, f)

    with open(os.path.join(kernel_dir, "deployment.c"), "w") as f:
        write_kernel_deployment_c(conf, f)

    # caller may pass more directories than required
    # we just ignore them
    assert len(partition_dirs) >= len(conf.partitions)

    for i in range(len(conf.partitions)):
        with open(os.path.join(partition_dirs[i], "deployment.h"), "w") as f:
            write_partition_deployment_h(conf, i, f)
            
        with open(os.path.join(partition_dirs[i], "deployment.c"), "w") as f:
            write_partition_deployment_c(conf, i, f)

def write_kernel_deployment_h(conf, f):
    p = functools.partial(print, file=f)

    p("#ifndef __POK_KERNEL_GENERATED_DEPLOYMENT_H_")
    p("#define __POK_KERNEL_GENERATED_DEPLOYMENT_H_")
    
    p(COMMON_KERNEL_DEFINES)

    if conf.network:
        p("#define POK_NEEDS_PCI 1")
        p("#define POK_NEEDS_NETWORKING 1")
        p("#define POK_NEEDS_NETWORKING_VIRTIO 1")

    total_threads = (
        1 + # kernel thread
        1 + # idle thread
        sum(
            part.get_needed_threads()
            for part in conf.partitions
        )
    )

    p("#define POK_CONFIG_NB_THREADS %d" % total_threads)

    p("#define POK_CONFIG_PARTITIONS_NTHREADS {%s}" % ", ".join(
        str(part.get_needed_threads()) for part in conf.partitions
    ))


    p("#define POK_CONFIG_NB_PARTITIONS %d" % len(conf.partitions))

    p("#define POK_CONFIG_NB_LOCKOBJECTS %d" % 
        sum(part.get_needed_lock_objects() for part in conf.partitions))
    p("#define POK_CONFIG_PARTITIONS_NLOCKOBJECTS {%s}" % ", ".join(
        str(part.get_needed_lock_objects()) for part in conf.partitions
    ))
    
    p("#define POK_CONFIG_PARTITIONS_SIZE {%s}" % ", ".join(
        str(part.size) for part in conf.partitions
    ))

    p("#define POK_CONFIG_SCHEDULING_NBSLOTS %d" % len(conf.slots))

    p("#define POK_CONFIG_SCHEDULING_MAJOR_FRAME %d" %
        sum(slot.duration for slot in conf.slots)
    )

    n_sampling_ports = len(conf.get_all_sampling_ports())
    n_queueing_ports = len(conf.get_all_queueing_ports())

    if n_sampling_ports > 0:
        p("#define POK_NEEDS_PORTS_SAMPLING 1")
        p("#define POK_CONFIG_NB_SAMPLING_PORTS %d" % n_sampling_ports)
    if n_queueing_ports > 0:
        p("#define POK_NEEDS_PORTS_QUEUEING 1")
        p("#define POK_CONFIG_NB_QUEUEING_PORTS %d" % n_queueing_ports)

    if conf.test_support_print_when_all_threads_stopped:
        p("#define POK_TEST_SUPPORT_PRINT_WHEN_ALL_THREADS_STOPPED 1")

    p("#endif") # __POK_KERNEL_GENERATED_DEPLOYMENT_H_ 

def write_kernel_deployment_c(conf, f):
    p = functools.partial(print, file=f)
    
    p('#include "deployment.h"')
    if len(conf.get_all_ports()) > 0:
        write_kernel_deployment_c_ports(conf, f)

    write_kernel_deployment_c_hm_tables(conf, f)

    if conf.network:
        #if conf.network.mac != None:
        #    p("static const unsigned char mac[] = %s;" % conf.network.mac_to_string());
        #    p("const unsigned char *pok_network_mac_address = mac;")
        #else:
        #    p("const unsigned char *pok_network_mac_address = NULL;")

        p("const uint32_t pok_network_ip_address = %s;" % hex(int(conf.network.ip)))

    p("const pok_sched_slot_t pok_module_sched[POK_CONFIG_SCHEDULING_NBSLOTS] = {")
    for slot in conf.slots:
        if isinstance(slot, TimeSlotSpare):
            p(TIMESLOT_SPARE_TEMPLATE % dict(
                duration=slot.duration,
            ))
        elif isinstance(slot, TimeSlotPartition):
            p(TIMESLOT_PARTITION_TEMPLATE % dict(
                duration=slot.duration,
                partition=slot.partition,
                periodic_processing_start="TRUE" if slot.periodic_processing_start else "FALSE"
            ))
        elif isinstance(slot, TimeSlotNetwork):
            p(TIMESLOT_NETWORKING_TEMPLATE % dict(
                duration=slot.duration
            ))
        else:
            raise TypeError("unrecognized slot type") 

    p("};")
        

def write_kernel_deployment_c_ports(conf, f):
    p = functools.partial(print, file=f)

    p("#include <middleware/port.h>")

    # various misc. functions

    def get_partition(port):
        for i, part in enumerate(conf.partitions):
            if port in part.ports:
                return i
        raise ValueError

    def get_internal_port_name(port, suffix=""):
        # the name that is used for static variables
        # including port data, channels, etc.
        
        if isinstance(port, SamplingPort):
            return "sp_%d_%s" % (all_sampling_ports.index(port), suffix)
        if isinstance(port, QueueingPort):
            return "qp_%d_%s" % (all_queueing_ports.index(port), suffix)

        assert False

    def get_internal_chan_name(chan, suffix=""):
        for i, c in enumerate(conf.channels):
            if c == chan:
                return "chan_%d_%s" % (i, suffix)

        assert False

    def get_internal_conn_name(conn, suffix=""):
        for i, chan in enumerate(conf.channels):
            if chan.src == conn:
                return "chan_%d_src_%s" % (i, suffix)
            if chan.dst == conn:
                return "chan_%d_dst_%s" % (i, suffix)

        assert False

    def get_port_index(port):
        # returns port index in port array
        # queueing and sampling port have distinct "indexspaces"

        if isinstance(port, SamplingPort):
            return all_sampling_ports.index(port)
        if isinstance(port, QueueingPort):
            return all_queueing_ports.index(port)

        assert False
        
    def get_connection_string(conn):
        if isinstance(conn, LocalConnection):
            return PORT_CONNECTION_LOCAL_TEMPLATE % dict(
                port_id=get_port_index(conn.port)
            )
        elif isinstance(conn, UDPConnection):
            addr = hex(int(conn.host))
            port = conn.port

            return PORT_CONNECTION_UDP_TEMPLATE % (
                "&" + 
                get_internal_conn_name(conn, "udp_misc")
            )
        else:
            raise TypeError(type(conn))
    

    all_ports = [
        port
        for part in conf.partitions
        for port in part.ports
    ]

    all_sampling_ports = [
        port
        for port in all_ports
        if isinstance(port, SamplingPort)
    ]

    all_queueing_ports = [
        port
        for port in all_ports
        if isinstance(port, QueueingPort)
    ]

    # print static data storage
    for i, port in enumerate(all_sampling_ports):
        p(SAMPLING_PORT_DATA_TEMPLATE % dict(
            varname=get_internal_port_name(port, "data"),
            max_message_size=port.max_message_size,
        ))

    for i, port in enumerate(all_queueing_ports):
        p(QUEUEING_PORT_DATA_TEMPLATE % dict(
            varname=get_internal_port_name(port, "data"),
            max_message_size=port.max_message_size,
            max_nb_messages=port.max_nb_messages,
        ))

    # static storage for sampling/queueing send buffers
    for i, conn in enumerate(conf.channels):
        if not isinstance(conn.dst, UDPConnection):
            continue
        
        # it's for sure
        assert isinstance(conn.src, LocalConnection)

        varname = get_internal_conn_name(conn.dst, "udp_misc")
        max_message_size = conn.src.port.max_message_size

        if isinstance(conn.src.port, SamplingPort):
            p(SAMPLING_PORT_UDP_SEND_BUFFER_TEMPLATE % dict(
                varname=varname,
                max_message_size=max_message_size,
                ip=hex(int(conn.dst.host)),
                port=conn.dst.port,
            ))
        elif isinstance(conn.src.port, QueueingPort):
            max_nb_messages = conn.src.port.max_nb_messages

            p(QUEUEING_PORT_UDP_SEND_BUFFER_TEMPLATE % dict(
                varname=varname,
                max_message_size=max_message_size,
                max_nb_messages=max_nb_messages,
                ip=hex(int(conn.dst.host)),
                port=conn.dst.port,
            ))
        else:
            assert False

    # static storage for sampling/queueing recv info (no buffer, though)
    for i, conn in enumerate(conf.channels):
        if not isinstance(conn.src, UDPConnection):
            continue

        # it's for sure
        assert isinstance(conn.dst, LocalConnection)

        varname = get_internal_conn_name(conn.src, "udp_misc")

        if isinstance(conn.dst.port, SamplingPort):
            p(SAMPLING_PORT_UDP_RECV_BUFFER_TEMPLATE % dict(
                varname = varname,
                ip=hex(int(conn.src.host)),
                port=conn.src.port,
            ))
        elif isinstance(conn.dst.port, QueueingPort):
            p(QUEUEING_PORT_UDP_RECV_BUFFER_TEMPLATE % dict(
                varname = varname,
                ip=hex(int(conn.src.host)),
                port=conn.src.port,
            ))
        else:
            assert False

    # print non-static definitions
    p("pok_port_sampling_t pok_sampling_ports[] = {")
    for i, port in enumerate(all_sampling_ports):

        p(SAMPLING_PORT_TEMPLATE % dict(
            name=_c_string(port.name),
            partition=get_partition(port),
            direction=_get_port_direction(port),
            max_message_size=port.max_message_size,
            data="&" + get_internal_port_name(port, "data")
        ))
    p("};")

    
    p("pok_port_queueing_t pok_queueing_ports[] = {")
    for i, port in enumerate(all_queueing_ports):

        p(QUEUEING_PORT_TEMPLATE % dict(
            name=_c_string(port.name),
            partition=get_partition(port),
            direction=_get_port_direction(port),
            max_message_size=port.max_message_size,
            max_nb_messages=port.max_nb_messages,
            data="&" + get_internal_port_name(port, "data"),
            data_stride="sizeof(%s[0])" % get_internal_port_name(port, "data")
        ))
    p("};")

    def print_channels(predicate, variable_name):
        p("pok_port_channel_t %s[] = {" % variable_name)
        for channel in conf.channels:
            if not predicate(channel): continue

            p("{")
            p(".src = %s," % get_connection_string(channel.src))
            p(".dst = %s," % get_connection_string(channel.dst))
            p("},")

        p("{")
        p(".src = %s," % PORT_CONNECTION_NULL)
        p(".dst = %s," % PORT_CONNECTION_NULL)
        p("},")

        p("};")

    print_channels(lambda c: c.is_queueing(), "pok_queueing_port_channels")
    print_channels(lambda c: c.is_sampling(), "pok_sampling_port_channels")


def write_kernel_deployment_c_hm_tables(conf, f):
    p = functools.partial(print, file=f)

    p('#include <core/error.h>')

    for i, part in enumerate(conf.partitions):
        p("static const pok_error_hm_partition_t partition_hm_table%d[] = {" % i)

        for error, action in part.hm_table:
            p("  {%s, %s}," % (error, action))

        p("  {POK_ERROR_KIND_INVALID, POK_ERROR_ACTION_IGNORE} /* sentinel value */")

        p("};")
        p("")

    p("const pok_error_hm_partition_t * const pok_partition_hm_tables[POK_CONFIG_NB_PARTITIONS] = {")
    for i in range(len(conf.partitions)):
        p("  partition_hm_table%d," % i)
    p("};")


def write_partition_deployment_h(conf, partition_idx, f):
    p = functools.partial(print, file=f)

    p("#ifndef __POK_USER_GENERATED_DEPLOYMENT_H_")
    p("#define __POK_USER_GENERATED_DEPLOYMENT_H_")
    
    p(COMMON_PARTITION_DEFINES)

    part = conf.partitions[partition_idx]

    if part.num_arinc653_buffers > 0:
        p("#define POK_NEEDS_BUFFERS 1")
        p("#define POK_NEEDS_ARINC653_BUFFER 1")
        p("#define POK_CONFIG_NB_BUFFERS %d" % part.num_arinc653_buffers)

        p("#define POK_CONFIG_BUFFER_DATA_SIZE %d" % part.buffer_data_size)
        
    if part.num_arinc653_blackboards > 0:
        p("#define POK_NEEDS_BLACKBOARDS 1")
        p("#define POK_NEEDS_ARINC653_BLACKBOARD 1")
        p("#define POK_CONFIG_NB_BLACKBOARDS %d" % part.num_arinc653_buffers)
        p("#define POK_CONFIG_BLACKBOARD_DATA_SIZE %d" % part.blackboard_data_size)

    if part.num_arinc653_semaphores > 0:
        p("#define POK_NEEDS_ARINC653_SEMAPHORE 1")
        p("#define POK_CONFIG_ARINC653_NB_SEMAPHORES %d" % part.num_arinc653_semaphores)

    if part.num_arinc653_events > 0:
        p("#define POK_NEEDS_ARINC653_EVENT 1")
        p("#define POK_CONFIG_ARINC653_NB_EVENTS %d" % part.num_arinc653_events)
        p("#define POK_CONFIG_NB_EVENTS %d" % part.num_arinc653_events)

    p("#define POK_CONFIG_NB_THREADS %d" % part.get_needed_threads())

    p("#endif")

def write_partition_deployment_c(conf, partition_idx, f):
    p = functools.partial(print, file=f)

    p('#include "deployment.h"')