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
 *
 * This file also incorporates work covered by POK License.
 * Copyright (c) 2007-2009 POK team
 */

#include <config.h>

#ifdef POK_NEEDS_ERROR_HANDLING

#include <types.h>
#include <core/error_arinc.h>
#include <core/partition_arinc.h>
#include <core/debug.h>
#include <core/sched_arinc.h>

#include <assert.h>
#include <libc.h>
#include <core/uaccess.h>
#include "thread_internal.h"
#include <cons.h>

static inline pok_error_kind_t get_error_kind(pok_error_id_t error_id)
{
    return current_partition_arinc->
        thread_error_info->map[error_id].error_kind;
}

static inline const char* get_error_description(pok_error_id_t error_id)
{
    return current_partition_arinc->
        thread_error_info->map[error_id].error_description;
}

const char* error_thread_name = "Error_Handler";

#define ERROR_THREAD_PRIORITY 254

pok_ret_t pok_error_thread_create (uint32_t stack_size, void* __user entry)
{
    pok_thread_t* t;
    pok_partition_arinc_t* part = current_partition_arinc;

    /**
     * We can create a thread only if the partition is in INIT mode
     */
    if (part->mode == POK_PARTITION_MODE_NORMAL) {
        return POK_ERRNO_PARTITION_MODE;
    }

    if(part->thread_error)
        return POK_ERRNO_EXISTS;

    if (part->nthreads_used == part->nthreads) {
        return POK_ERRNO_TOOMANY;
    }

    // do at least basic check of entry point
    if (!jet_check_access_exec(entry)) {
        return POK_ERRNO_PARAM;
    }


    t = &part->threads[part->nthreads_used];

    strncpy(t->name, error_thread_name, MAX_NAME_LENGTH);
    t->entry = entry;
    t->base_priority = ERROR_THREAD_PRIORITY;
    t->period = POK_TIME_INFINITY;
    t->time_capacity = POK_TIME_INFINITY; // TODO: support deadline for error handler
    t->deadline = DEADLINE_SOFT;
    t->user_stack_size = stack_size;

    if(!thread_create(t))
       return POK_ERRNO_UNAVAILABLE;

    // Update kernel shared data
    part->kshd->error_thread_id = part->nthreads_used;

    part->nthreads_used++;

    part->thread_error = t;

    return POK_ERRNO_OK;
}

// Should be called with local preemption disabled.
static void take_fixed_action(pok_error_action_t action, pok_error_id_t error_id)
{
    /*************Modified JB****************/
    //If the partition restarts, write the reason why
    if(action == POK_ERROR_ACTION_COLD_START || action == POK_ERROR_ACTION_WARM_START){
        pok_partition_arinc_t* part = current_partition_arinc;
        pok_cons_write("[KERNEL] Restarting partition ", 30);
        pok_cons_write(part->base_part.name, strlen(part->base_part.name));
        pok_cons_write(" due to: ", 9);
        const char *msg = get_error_description(error_id);
        pok_cons_write(msg, strlen(msg));
        pok_cons_write("\n", 1);
    }
    /****************************************/

    switch (action) {
        case POK_ERROR_ACTION_IGNORE:
            // TODO not all kinds of errors can be ignored just like that
            return;
        case POK_ERROR_ACTION_IDLE:
            pok_partition_arinc_idle();
            unreachable();
            break;
        case POK_ERROR_ACTION_COLD_START:
            pok_partition_arinc_reset(POK_PARTITION_MODE_INIT_COLD);
            assert(FALSE && "this's supposed to be unreachable");
            break;
        case POK_ERROR_ACTION_WARM_START:
            // TODO: Instead of assert() this should be another error or other actions.
            assert(current_partition_arinc->mode != POK_PARTITION_MODE_INIT_COLD);

            pok_partition_arinc_reset(POK_PARTITION_MODE_INIT_WARM);
            unreachable();
            break;
        default:
            unreachable();
    }
}

/*
 * Attempt to process error at partition level or above.
 *
 * Return TRUE or doesn't return at all if error has been processed.
 *
 * Return FALSE if error should be processed at process level.
 *
 * Should be called with local preemption disabled.
 */
static pok_bool_t process_error_partition(pok_system_state_t system_state,
    pok_error_id_t error_id)
{
    pok_partition_arinc_t* part = current_partition_arinc;
    pok_error_action_t action;

    switch(system_state)
    {
    case POK_SYSTEM_STATE_USER:
#ifdef POK_NEEDS_ERROR_HANDLING
        if(pok_error_level_select(part->partition_hm_selector,
            system_state, error_id)
            && part->mode == POK_PARTITION_MODE_NORMAL
            && part->thread_error) return FALSE;
        // Fallback
    case POK_SYSTEM_STATE_ERROR_HANDLER: // This state can only be viewed with error handling enabled.
        // Fallback
#endif /* POK_NEEDS_ERROR_HANDLING */
    case POK_SYSTEM_STATE_OS_PART:
        break;
    default:
        pok_fatal("Unexpected partition state while error");
    }

    action = part->partition_hm_table->actions[system_state][error_id];

    take_fixed_action(action, error_id);

    if(part->nthreads_unrecoverable)
    {
        part->nthreads_unrecoverable = 0;
        while(1)
            process_error_partition(POK_SYSTEM_STATE_OS_PART,
                POK_ERROR_ID_UNHANDLED_INT);
    }

    return TRUE;
}

/*
 * Set state of the thread as unrecoverable.
 *
 * Should be called before emitting error.
 *
 * Should be called with local preemption disabled.
 */
static void thread_set_unrecoverable(pok_thread_t* thread)
{
    if(!thread->is_unrecoverable)
    {
        thread->is_unrecoverable = TRUE;
        current_partition_arinc->nthreads_unrecoverable++;
    }
}

#ifdef POK_NEEDS_ERROR_HANDLING

/*
 * Set error bit in the thread.
 *
 * Should be executed with local preemption disabled.
 */
static void thread_add_error_bit(pok_thread_t* thread,
    pok_thread_error_bits_t bit)
{
    assert((thread->error_bits & bit) == 0);

    thread->error_bits |= bit;

    if(list_empty(&thread->error_elem))
    {
        list_add_tail(&thread->error_elem, &current_partition_arinc->error_list);
        pok_sched_local_invalidate();
    }
}

/*
 * Clear error bit in the thread.
 *
 * Note, that other information about the error doesn't destroyed.
 *
 * Should be executed with local preemption disabled.
 */
static void thread_clear_error_bit(pok_thread_t* thread,
    pok_thread_error_bits_t bit)
{
    assert((thread->error_bits & bit) == bit);
    assert(!list_empty(&thread->error_elem));

    thread->error_bits -= bit;

    if(!thread->error_bits)
    {
        list_del_init(&thread->error_elem);
    }
}


/*
 * Emit sync error for non-error-handler thread.
 *
 * Should be called only for process-level errors.
 *
 * system_state is POK_SYSTEM_STATE_USER.
 *
 * Additional information about error should already be filled
 * in accordance with description of POK_THREAD_ERROR_BIT_SYNC.
 *
 * Should be called with local preemption disabled.
 */
static void thread_emit_sync_error(pok_thread_t* thread,
    pok_error_id_t error_id, void* __user failed_addr)
{
    current_partition_arinc->sync_error = error_id;
    current_partition_arinc->sync_error_failed_addr = failed_addr;

    thread_add_error_bit(thread, POK_THREAD_ERROR_BIT_SYNC);
}

#endif /* POK_NEEDS_ERROR_HANDLING */

void pok_partition_arinc_process_error(
    pok_system_state_t partition_state,
    pok_error_id_t error_id,
    uint8_t preempt_local_disabled_old,
    void* failed_address)
{
    pok_partition_arinc_t* part = current_partition_arinc;

    // TODO: Take into account `preempt_local_disabled_old`

    if(process_error_partition(partition_state, error_id)) return;

#ifdef POK_NEEDS_ERROR_HANDLING

    assert(partition_state == POK_SYSTEM_STATE_USER);
    assert(part->thread_current != part->thread_error);

    pok_preemption_local_disable();

    thread_emit_sync_error(part->thread_current, error_id, (__user void*)failed_address);

    pok_preemption_local_enable();
#else
    assert(FALSE); // Unreachable
#endif /* POK_NEEDS_ERROR_HANDLING */
}

void pok_thread_emit_deadline_missed(pok_thread_t* thread)
{
    if(process_error_partition(POK_SYSTEM_STATE_USER,
        POK_ERROR_ID_DEADLINE_MISSED)) return;

#ifdef POK_NEEDS_ERROR_HANDLING
    thread_add_error_bit(thread, POK_THREAD_ERROR_BIT_DEADLINE);
#else
    assert(FALSE); // Unreachable
#endif /* POK_NEEDS_ERROR_HANDLING */
}

void pok_thread_emit_deadline_oor(pok_thread_t* thread)
{
    thread_set_unrecoverable(thread);

    if(process_error_partition(POK_SYSTEM_STATE_USER,
        POK_ERROR_ID_ILLEGAL_REQUEST)) return;

#ifdef POK_NEEDS_ERROR_HANDLING
    thread_add_error_bit(thread, POK_THREAD_ERROR_BIT_DEADLINE_OOR);
#else
    assert(FALSE); // Unreachable
#endif /* POK_NEEDS_ERROR_HANDLING */
}

#ifdef POK_NEEDS_ERROR_HANDLING
pok_ret_t pok_error_raise_application_error (const char* __user msg, size_t msg_size)
{
    pok_partition_arinc_t* part = current_partition_arinc;
    pok_thread_t* thread = part->thread_current;

    pok_system_state_t partition_state;
    const char* __kuser k_msg;

    if (msg_size > POK_ERROR_MAX_MSG_SIZE || msg_size == 0) {
        return POK_ERRNO_EINVAL;
    }

    k_msg = jet_user_to_kernel_ro(msg, msg_size);

    if (!k_msg) return POK_ERRNO_EFAULT;

    if(thread == part->thread_error)
    {
        partition_state = POK_SYSTEM_STATE_ERROR_HANDLER;
    }
    else
    {
        partition_state = POK_SYSTEM_STATE_USER;
    }

    if(process_error_partition(partition_state, POK_ERROR_ID_APPLICATION_ERROR))
        return POK_ERRNO_OK;

    assert(part->thread_error);
    assert(part->thread_current != part->thread_error);

    thread->wait_buffer.src = k_msg;
    thread->wait_len = msg_size;

    pok_preemption_local_disable();

    thread_emit_sync_error(thread, POK_ERROR_ID_APPLICATION_ERROR, NULL);

    pok_preemption_local_enable();

    return POK_ERRNO_OK;
}

pok_ret_t pok_error_raise_os_error (const char* __user msg, size_t msg_size)
{
    const char* __kuser k_msg;

    if (msg_size == 0) goto out; // Cannot return. Just ignore message.

    k_msg = jet_user_to_kernel_ro(msg, msg_size);

    if (!k_msg) goto out; // Cannot return. Just ignore message.

    jet_console_write(k_msg, msg_size);

out:
    if(process_error_partition(POK_SYSTEM_STATE_OS_PART, POK_ERROR_ID_APPLICATION_ERROR))
        return POK_ERRNO_OK;

    // Should never return.
    while(1)
        process_error_partition(POK_SYSTEM_STATE_OS_PART,
            POK_ERROR_ID_UNHANDLED_INT);
}


void error_check_after_handler(void)
{
    pok_partition_arinc_t* part = current_partition_arinc;
    if(part->nthreads_unrecoverable)
    {
        /*
         * Note, that we do not reset unrecoverable counter.
         *
         * Such a way partition's error processing mechanism treats
         * error as unrecoverable too.
         */
        process_error_partition(POK_SYSTEM_STATE_ERROR_HANDLER,
            POK_ERROR_ID_UNHANDLED_INT);
    }
}


/*
 * Fill information about error in user-space structure.
 *
 * Should be called with local preemption disabled.
 */
static void copy_error_to_user(pok_error_id_t error_id,
    pok_error_status_t* __kuser k_status, void* __kuser k_msg)
{
    const char* msg = get_error_description(error_id);
    size_t msg_size;

    msg_size = strnlen(msg, POK_ERROR_MAX_MSG_SIZE);
    if(msg_size != POK_ERROR_MAX_MSG_SIZE) msg_size++;

    memcpy(k_msg, msg, msg_size);
    k_status->msg_size = msg_size;
    k_status->error_kind = get_error_kind(error_id);
}

pok_ret_t pok_error_get (pok_error_status_t* __user status,
    void* __user msg)
{
    pok_partition_arinc_t* part = current_partition_arinc;
    pok_thread_t* thread;
    pok_thread_error_bits_t error_bits;

    void* __user failed_addr = NULL;

    if (part->thread_current != part->thread_error) {
        return POK_ERRNO_THREAD;
    }

    if(list_empty(&part->error_list)) return POK_ERRNO_UNAVAILABLE;

    pok_error_status_t* __kuser k_status = jet_user_to_kernel_typed(status);
    if(!k_status) return POK_ERRNO_EFAULT;

    void* __kuser k_msg = jet_user_to_kernel(msg, POK_ERROR_MAX_MSG_SIZE);
    if(!k_msg) return POK_ERRNO_EFAULT;

    thread = list_first_entry(&part->error_list, pok_thread_t, error_elem);

    error_bits = thread->error_bits;

    if(error_bits & POK_THREAD_ERROR_BIT_SYNC)
    {
         thread_clear_error_bit(thread, POK_THREAD_ERROR_BIT_SYNC);

         if(part->sync_error == POK_ERROR_ID_APPLICATION_ERROR)
         {
             k_status->msg_size = thread->wait_len;
             k_status->error_kind = get_error_kind(POK_ERROR_ID_APPLICATION_ERROR);
             memcpy(k_msg, thread->wait_buffer.src, thread->wait_len);
         }
         else
         {
             copy_error_to_user(part->sync_error, k_status, k_msg);
         }

         failed_addr = part->sync_error_failed_addr;
    }
    else if(error_bits & POK_THREAD_ERROR_BIT_DEADLINE)
    {
        thread_clear_error_bit(thread, POK_THREAD_ERROR_BIT_DEADLINE);

        copy_error_to_user(POK_ERROR_ID_DEADLINE_MISSED, k_status, k_msg);
    }

    else if(error_bits & POK_THREAD_ERROR_BIT_DEADLINE_OOR)
    {
        thread_clear_error_bit(thread, POK_THREAD_ERROR_BIT_DEADLINE_OOR);

        copy_error_to_user(POK_ERROR_ID_ILLEGAL_REQUEST, k_status, k_msg);
    }
    else
    {
        unreachable();
    }

    k_status->failed_thread = thread - part->threads;
    k_status->failed_addr =  (uintptr_t)failed_addr;

    return POK_ERRNO_OK;
}

void error_ignore_sync(void)
{
    pok_partition_arinc_t* part = current_partition_arinc;

    if(list_empty(&part->error_list)) return;

    // Synchronous error could be only the first in the list.
    pok_thread_t* thread = list_first_entry(&part->error_list, pok_thread_t, error_elem);

    if(thread->error_bits & POK_THREAD_ERROR_BIT_SYNC)
        thread_clear_error_bit(thread, POK_THREAD_ERROR_BIT_SYNC);
}
#endif /* POK_NEEDS_ERROR_HANDLING */

#endif

