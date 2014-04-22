/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team
 *
 * This file also incorporates work covered by the following 
 * copyright and license notice:
 *
 *  Copyright (C) 2013-2014 Maxim Malkov, ISPRAS <malkov@ispras.ru> 
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created by julien on Thu Jan 15 23:34:13 2009
 */

#ifndef __POK_THREAD_H__
#define __POK_THREAD_H__

#include <core/dependencies.h>

#ifdef POK_NEEDS_THREADS

#include <types.h>
#include <errno.h>
#include <core/syscall.h>

#define POK_THREAD_DEFAULT_PRIORITY 42

#define POK_DEFAULT_STACK_SIZE 2048

// must match to kernel/include/core/sched.h
typedef enum
{
  POK_STATE_STOPPED = 0,
  POK_STATE_RUNNABLE = 1,
  POK_STATE_WAITING = 2,
  POK_STATE_LOCK = 3,
  POK_STATE_WAIT_NEXT_ACTIVATION = 4,
  POK_STATE_DELAYED_START = 5
} pok_state_t;

typedef enum {
    DEADLINE_SOFT,
    DEADLINE_HARD
} pok_deadline_t;

typedef struct
{
	 uint8_t      priority;
	 void*        entry;
	 int64_t      period;
	 pok_deadline_t      deadline;
	 int64_t      time_capacity;
	 uint32_t     stack_size;
} pok_thread_attr_t;

typedef struct 
{
        pok_thread_attr_t   attributes;
        uint64_t            deadline_time;
	pok_state_t         state;
        uint8_t             current_priority;
} pok_thread_status_t;


pok_ret_t       pok_thread_create (pok_thread_id_t *thread_id, const pok_thread_attr_t* attr);
pok_ret_t       pok_thread_sleep(int64_t ms);
pok_ret_t       pok_thread_sleep_until(pok_time_t ms);
pok_ret_t       pok_thread_lock(void);
pok_ret_t       pok_thread_unlock(pok_thread_id_t thread_id);
pok_ret_t       pok_thread_yield();
pok_thread_id_t pok_thread_current (void);
pok_ret_t       pok_thread_wait_infinite ();
pok_ret_t       pok_thread_attr_init (pok_thread_attr_t* attr);
pok_ret_t       pok_thread_period(void);
pok_ret_t       pok_thread_id (pok_thread_id_t* thread_id);
pok_ret_t       pok_thread_status(pok_thread_id_t thread_id, pok_thread_status_t* attr);
pok_ret_t       pok_thread_delayed_start(pok_thread_id_t thread_id, int64_t ms);
pok_ret_t       pok_thread_set_priority(pok_thread_id_t thread_id,  uint32_t priority);
pok_ret_t       pok_thread_resume(pok_thread_id_t thread_id);

#define pok_thread_sleep_until(time) pok_syscall2(POK_SYSCALL_THREAD_SLEEP_UNTIL,(uint32_t)time,0)

#define pok_thread_wait_infinite() pok_thread_suspend()

#define pok_thread_suspend() pok_syscall2(POK_SYSCALL_THREAD_SUSPEND,NULL,NULL)

#define pok_thread_suspend_target(thread_id) pok_syscall2(POK_SYSCALL_THREAD_SUSPEND_TARGET,thread_id,0)

/*
 * Similar to: pok_ret_t      pok_thread_suspend (void);
 */

#define pok_thread_restart(thread_id) pok_syscall2(POK_SYSCALL_THREAD_RESTART,thread_id,0)
/*
 * similar to:
 * pok_ret_t      pok_thread_restart (uint32_t thread_id);
 */

#define pok_thread_stop_self() pok_syscall2(POK_SYSCALL_THREAD_STOPSELF, 0, 0)
/*
 * similar to:
 * pok_ret_t      pok_thread_stop_self ();
 */

#define pok_thread_stop(id) pok_syscall2(POK_SYSCALL_THREAD_STOP,id,NULL)
/*
 * similar to: pok_ret_t      pok_thread_stop (const uint32_t tid);
 */

#define pok_partition_inc_lock_level(lock_level) pok_syscall2(POK_SYSCALL_PARTITION_INC_LOCK_LEVEL,(uint32_t)lock_level,0)

#define pok_partition_dec_lock_level(lock_level) pok_syscall2(POK_SYSCALL_PARTITION_DEC_LOCK_LEVEL,(uint32_t)lock_level,0)

#define pok_thread_yield() pok_syscall2(POK_SYSCALL_THREAD_YIELD, 0, 0)

#endif /* __POK_NEEDS_THREADS */
#endif /* __POK_THREAD_H__ */
