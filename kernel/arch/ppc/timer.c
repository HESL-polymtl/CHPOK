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

#include <errno.h>
#include "bsp/bsp.h"
#include <core/time.h>
#include <core/sched.h>
#include <core/debug.h>
#include "reg.h"
#include "timer.h"
#include <cons.h>
#include <asp/entries.h>

/* First value of decrementer.  */
static uint64_t time_first;


/* Last time when decr was set.  */
static uint64_t time_last;

/* Decrementer optimal value.  */
static uint32_t time_inter;

/*
 * Hardcoded calendar time at the beginning of the OS loading.
 * 
 * Could be obtained on https://www.timeanddate.com/.
 */
static time_t base_calendar_time = 1480330081; // On 28.11.2016

static uint64_t get_timebase(void)
{
    while (1) {
        uint32_t upper = mfspr(SPRN_TBRU);
        uint32_t lower = mfspr(SPRN_TBRL);

        // since it's two part register,
        // there's a possible race condition
        // we avoid it here with a loop
        if (upper == mfspr(SPRN_TBRU)) {
            return (((uint64_t) upper) << 32) | lower;
        }
    }
}

/* Compute new value for the decrementer.  If the value is in the future,
   sets the decrementer else returns an error.  */
static int set_decrementer(void)
{
  uint64_t time_new = time_last + time_inter;
  uint64_t time_cur = get_timebase();
  int32_t delta = time_new - time_cur;

  time_last = time_new;

  if (delta < 0)
  {
    // that delta already expired
    return POK_ERRNO_EINVAL;
  }
  else
  {
    mtspr(SPRN_DEC, delta);
    return POK_ERRNO_OK;
  }
}

/* Called by the interrupt handled.  */
void pok_arch_decr_int (void)
{
  int err;

  // clear pending intrerrupt
  mtspr(SPRN_TSR, TSR_DIS);

  do
  {
    err = set_decrementer();
  } while (err != POK_ERRNO_OK);

  jet_on_tick();
}

void ja_time_init (void)
{
  time_inter = pok_bsp.timebase_freq / POK_TIMER_FREQUENCY;
  printf("Timer interval: %lu\n", (long unsigned int)time_inter);
  time_first = time_last = get_timebase ();
  set_decrementer();

  mtspr(SPRN_TCR, TCR_DIE); // enable decrementer
}

pok_time_t ja_system_time(void)
{
  return ((get_timebase() - time_first) / time_inter) * 1000000;
}

time_t ja_calendar_time(void)
{
  return base_calendar_time + (time_t)(ja_system_time() / 1000000000);
}
