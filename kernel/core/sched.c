/*
 * Institute for System Programming of the Russian Academy of Sciences
 * Copyright (C) 2016 ISPRAS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, Version 3.
 *
 * This program is distributed in the hope # that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License version 3 for more details.
 */

#include <config.h>

#include <types.h>
#include <asp/arch.h>

#include <core/time.h>
#include <core/sched.h>
#include <core/thread.h>

#include <core/partition.h>

#include <dependencies.h>

#include <core/debug.h>
#include <core/instrumentation.h>
#include <core/error.h>

#include <assert.h>

#include <cswitch.h>
#include <core/space.h>

static pok_time_t first_frame_starts; // Time when first major frame is started.

static pok_time_t            pok_sched_next_deadline;
static pok_time_t            pok_sched_next_major_frame;
static uint8_t               pok_sched_current_slot = 0; /* Which slot are we executing at this time ?*/

pok_partition_t* current_partition = NULL;

#if POK_NEEDS_GDB
struct jet_interrupt_context* global_thread_stack = NULL;
volatile pok_bool_t pok_in_user_space = 0;
#endif

#ifdef POK_NEEDS_MONITOR
/*
 * Whether current partition has `.is_paused` flag set.
 *
 * This flag affects on the place, where currently used context should
 * be stored on context switch.
 * From the other side, the flag can be changed *outside* of scheduler.
 */
pok_bool_t current_partition_is_paused;

struct jet_context* idle_sp;
uint32_t idle_stack;

static void idle_function(void)
{
    pok_preemption_enable();

    ja_inf_loop();
}
#endif

/*
 * Pointer to the store area for last executed (user) thread.
 *
 * If no thread has been executed yet, or the last one dies, this is NULL.
 */
struct jet_fp_store* fp_store_last = NULL;

// Reset partition state, so scheduler may restart.
static void pok_partition_reset(pok_partition_t* part)
{
    part->sp = 0;
    part->restarted_externally = TRUE;
    part->partition_generation++;
    if(part->partition_generation == 0)
    {
        part->partition_generation = 1;
    }

    part->partition_event_begin = part->partition_event_end = 0;
}


static pok_bool_t sched_need_recheck;

static void start_partition(void)
{
    pok_partition_t* part = current_partition;
    // Initialize state for started partition.
    part->is_event = FALSE;
    part->partition_event_begin = part->partition_event_end = 0;

    part->preempt_local_disabled = 1;

    /* It is safe to enable preemption on new stack. */
    pok_preemption_enable();

    if(part->part_ops && part->part_ops->start)
        part->part_ops->start();

    ja_inf_loop();
}

/* Switch within current partition, if needed. */
static void intra_partition_switch(void)
{
    struct jet_context** old_sp = &current_partition->sp;

#ifdef POK_NEEDS_MONITOR
    struct jet_context** new_sp = old_sp;
    if(current_partition_is_paused) old_sp = &idle_sp;
    if(current_partition->is_paused) new_sp = &idle_sp;

    if(old_sp != new_sp)
    {
        /* Need to switch context */
        current_partition_is_paused = current_partition->is_paused;

        if(*old_sp == NULL)
        {
            /*
             * Restart is requested by currently executed context.
             * (new context is idle, so it doesn't need restart.)
             *
             * Perform jump instead of switch.
             */
             jet_context_jump(*new_sp);
             return;
        }
        else if(*new_sp == NULL)
        {
            /*
             * Restart is requested by new context.
             * (current context is idle, so it doesn't need restart.)
             *
             * Need to initialize new context before switch.
             */
            *new_sp = jet_context_init(
                current_partition->initial_sp,
                &start_partition);
        }
        jet_context_switch(old_sp, *new_sp);
        return;
    }

#endif /* POK_NEEDS_MONITOR */
    // old_sp == new_sp
    if(*old_sp == NULL) /* Same context, restart requested. */
    {
        jet_context_restart_and_save(current_partition->initial_sp,
            &start_partition,
            old_sp);
    }
}


/* Switch to the new partition. */
static void inter_partition_switch(pok_partition_t* part)
{
    struct jet_context** old_sp = &current_partition->sp;
    struct jet_context** new_sp = &part->sp;
#if POK_NEEDS_GDB
    current_partition->entry_sp = global_thread_stack;
#endif
    current_partition = part;

    if(part->space_id != 0)
        pok_space_switch(part->space_id);
    else
        pok_space_switch(0); // TODO: This should disable all user space tables
#ifdef POK_NEEDS_MONITOR
    if(current_partition_is_paused) old_sp = &idle_sp;
    if(part->is_paused)
    {
        assert(idle_sp); // Idle sp shouldn't be 0.
        new_sp = &idle_sp;
    }

    if(old_sp == new_sp)
    {
        // Idle thread is continued. Nothing to do.
        return;
    }

    current_partition_is_paused = part->is_paused;
#endif /* POK_NEEDS_MONITOR */
    // old_sp != new_sp

    if(*new_sp == NULL)
    {
        /*
         * Restart is requested by new context.
         * (current context is different)
         *
         * Need to initialize new context before switch.
         */
        *new_sp = jet_context_init(part->initial_sp, &start_partition);
    }

    if(*old_sp == NULL)
    {
        jet_context_jump(*new_sp);
    }
    else
    {
        jet_context_switch(old_sp, *new_sp);
    }
}


void pok_sched_restart (void)
{
    struct jet_context** new_sp;

    first_frame_starts = jet_system_time();
#ifdef POK_NEEDS_MONITOR
    idle_sp = jet_context_init(idle_stack, &idle_function);
#endif /*POK_NEEDS_MONITOR */

    for_each_partition(&pok_partition_reset);

    sched_need_recheck = 0; // Acquire semantic
    barrier();

    // Navigate to the first slot
    pok_sched_current_slot = 0;
    pok_sched_next_major_frame = first_frame_starts + pok_config_scheduling_major_frame;
    pok_sched_next_deadline = pok_module_sched[0].duration + first_frame_starts;

    current_partition = pok_module_sched[0].partition;

    new_sp = &current_partition->sp;
#ifdef POK_NEEDS_MONITOR
    if(current_partition->is_paused) new_sp = &idle_sp;
    current_partition_is_paused = current_partition->is_paused;
#endif /*POK_NEEDS_MONITOR */
    if(*new_sp == 0)
    {
        *new_sp = jet_context_init(current_partition->initial_sp,
            &start_partition);
    }

    if(current_partition->space_id != 0xff)
        pok_space_switch(current_partition->space_id);
    else
        pok_space_switch(0xff); // TODO: This should disable all user space tables

    kernel_state = POK_SYSTEM_STATE_OS_PART;

    jet_context_jump(*new_sp);
}

void pok_sched_start (void)
{
#ifdef POK_NEEDS_MONITOR
    idle_stack = pok_stack_alloc(KERNEL_STACK_SIZE_DEFAULT);
#endif /*POK_NEEDS_MONITOR */
    pok_sched_restart();
}

/* Static variables used to trace with qemu */
#define QEMU_TRACING 0

#if QEMU_TRACING
static int started = 0;
static int ended = 0;
static int nb_major_time_frame = 0;
#endif /* QEMU_TRACE */
/********************************************/

/*
 * Perform scheduling.
 *
 * Should be called with preemption disabled.
 */
static void pok_sched(void)
{
    pok_partition_t* part = current_partition;
    pok_partition_t* new_partition;
    pok_time_t now;

    if(!flag_test_and_reset(sched_need_recheck)) return;

    now = jet_system_time();

    if(pok_sched_next_deadline > now) goto same_partition;

    pok_sched_current_slot = (pok_sched_current_slot + 1);
    if(pok_sched_current_slot == pok_module_sched_n)
    {
        pok_sched_next_major_frame += pok_config_scheduling_major_frame;
        pok_sched_current_slot = 0;

        /********Added code for qemu trace ************/
#if QEMU_TRACING
        ++nb_major_time_frame;
        if(started && !ended && (nb_major_time_frame >= 10))
        {
            __asm__ __volatile__(".byte 0x0F, 0xA7, 0x00, 0x00\n");
            ended = 1;
        }
        else if(!started)
        {
            __asm__ __volatile__(".byte 0x0F, 0xA6, 0x00, 0x00\n");
            started = 1;
        }
#endif /* QEMU_TRACING */
        /*********************************************/
    }
    pok_sched_next_deadline += pok_module_sched[pok_sched_current_slot].duration;

    new_partition = pok_module_sched[pok_sched_current_slot].partition;

    if(new_partition == part) goto same_partition;

    inter_partition_switch(new_partition);

    /*
     * If original partition's timer expires during other partition work,
     * deliver timer event now.
     */
    now = jet_system_time();
    if(part->timer != 0 && part->timer <= now)
    {
        pok_partition_add_event(part, JET_PARTITION_EVENT_TYPE_TIMER, 0);
        part->timer = 0;
    }

    return;

same_partition:
    intra_partition_switch();
}

void pok_preemption_disable(void)
{
    assert(ja_preempt_enabled());

    ja_preempt_disable();
}


void pok_preemption_enable(void)
{
    assert(!ja_preempt_enabled());

    pok_sched();
    if(current_partition->preempt_local_disabled
        || !current_partition->is_event)
    {
        // Partition doesn't require notifications. Common case.
        ja_preempt_enable();
        return;
    }

    current_partition->preempt_local_disabled = 1;

    // Until partition "consume" all state bits or enables preemption.
    do
    {
        ja_preempt_enable();

        current_partition->part_sched_ops->on_event();

        ja_preempt_disable();
    } while(current_partition->preempt_local_disabled
        && current_partition->is_event);

    current_partition->preempt_local_disabled = 0;
    ja_preempt_enable();
}

void __pok_preemption_enable(void)
{
    assert(!ja_preempt_enabled());
    ja_preempt_enable();
}


/*
 * Forward implementation, which iterates over all slots.
 *
 * May be optimized in the future.
 */
pok_time_t get_next_periodic_processing_start(void)
{
    int i;

    pok_time_t offset = pok_sched_next_deadline;

    // check all time slots
    // note that we ignore current activation of _this_ slot
    // e.g. if we're currently in periodic processing window,
    // and it's the only one in schedule, we say that next one
    // will be major frame time units later
    int time_slot_index = pok_sched_current_slot;

    for (i = 0; i < pok_module_sched_n; i++) {

        time_slot_index++;
        if(time_slot_index == pok_module_sched_n) time_slot_index = 0;

        const pok_sched_slot_t *slot = &pok_module_sched[time_slot_index];

        if (slot->periodic_processing_start && slot->partition == current_partition) {
            return offset;
        }

        offset += slot->duration;
    }

    assert(FALSE && "Couldn't find next periodic processing window (configurator shouldn't have allowed that)");
}

void pok_sched_on_time_changed(void)
{
    assert(!ja_preempt_enabled());

    pok_partition_t* part = current_partition;
    sched_need_recheck = TRUE;

#if POK_NEEDS_GDB
    pok_bool_t in_user_space = pok_in_user_space;

    if(in_user_space)
    {
        part->entry_sp_user = global_thread_stack;
        pok_in_user_space = FALSE;
    }
#endif /* POK_NEEDS_GDB */
    pok_sched();

    if(current_partition_is_paused) goto out;

    pok_bool_t preempt_local_disabled_old = current_partition->preempt_local_disabled;

    pok_time_t current_time = jet_system_time();

    if(part->timer != 0 && part->timer <= current_time)
    {
        pok_partition_add_event(part, JET_PARTITION_EVENT_TYPE_TIMER, 0);
        part->timer = 0;
    }

    if(preempt_local_disabled_old || !part->is_event) goto out;
    // Emit events for partition.
    do
    {
        part->preempt_local_disabled = 1;
        ja_preempt_enable();
        part->part_sched_ops->on_event();
        ja_preempt_disable();
    } while(part->preempt_local_disabled && part->is_event);

    part->preempt_local_disabled = 0;

    // Still with disabled preemption. It is needed for returning from interrupt.

out:
#if POK_NEEDS_GDB
    // Restore user space indicator on return
    pok_in_user_space = in_user_space;
#else
    return;
#endif
}

static void pok_partition_return_user_common(void)
{
    pok_partition_t* part = current_partition;

    pok_preemption_disable();

    // Emit events for partition.
    while(part->preempt_local_disabled
        && part->is_event)
    {
        part->preempt_local_disabled = 1;
        ja_preempt_enable();
        part->part_sched_ops->on_event();
        ja_preempt_disable();
    }

    part->preempt_local_disabled = 0;
#if POK_NEEDS_GDB
    pok_in_user_space = TRUE;
#endif
}

void pok_partition_return_user(void)
{
    pok_partition_t* part = current_partition;

    pok_partition_return_user_common();

    assert(part->fp_store_current);

    if(fp_store_last != part->fp_store_current)
    {
        if(fp_store_last)
        {
            ja_fp_save(fp_store_last);
        }

        fp_store_last = part->fp_store_current;
        ja_fp_restore(fp_store_last);
    }
}

void pok_partition_jump_user(void (* __user entry)(void),
    jet_ustack_t stack_user,
    jet_stack_t stack_kernel)
{
    pok_partition_t* part = current_partition;

    pok_partition_return_user_common();

    assert(part->fp_store_current);

    if(fp_store_last && fp_store_last != part->fp_store_current)
    {
        ja_fp_save(fp_store_last);
    }

    fp_store_last = part->fp_store_current;
    ja_fp_init();

    jet_user_space_jump(
        stack_kernel,
        part->space_id,
        entry,
        (unsigned long)stack_user);
}

void pok_partition_restart(void)
{
    pok_partition_t* part = current_partition;
    assert(part);

    pok_preemption_disable();

    // Assign new generation for partition.
    part->partition_generation++;
    // For the case of overflow.
    if(part->partition_generation == 0) part->partition_generation = 1;

    part->sp = 0;
    sched_need_recheck = TRUE;

    pok_preemption_enable();

    unreachable();
}


void pok_sched_init(void)
{
    pok_partition_init(&partition_idle);
    partition_idle.initial_sp = pok_stack_alloc(4096);
}
