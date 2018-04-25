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
 * Created by julien on Thu Jan 15 23:34:13 2009
 */

#include <config.h>

#ifndef __POK_TIME_H__
#define __POK_TIME_H__

#include <types.h>
#include <errno.h>
#include <common.h>

#include <uapi/time.h>
#include <asp/time.h>

/**
 * The rate of the clock in POK
 */
#define POK_TIMER_FREQUENCY 10000

/* Return current system time, in nanoseconds. */
#define jet_system_time() ja_system_time()

/* Return calendar time, in seconds since Epoch. */
#define jet_calendar_time() ja_calendar_time()

pok_ret_t pok_clock_gettime (clockid_t clk_id, pok_time_t* __user val);

pok_ret_t jet_time(time_t* __user val);

#endif  /* __POK_TIME_H__ */
