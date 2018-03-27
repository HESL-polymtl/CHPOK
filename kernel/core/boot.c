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

/**
 * \file    core/boot.c
 * \author  Julien Delange
 * \brief   Boot function to start the kernel
 * \date    2008-2009
 */

#include <config.h>

#include <cons.h>

#include <core/time.h>
#include <core/thread.h>
#include <core/sched.h>
#include <core/partition.h>
#include <core/partition_arinc.h>
#include <core/channel.h>
#include <asp/entries.h>
#include <libc.h>

#include <core/instrumentation.h>

#ifdef POK_NEEDS_GDB
#include <gdb.h>
#endif

void jet_boot (void)
{
   kernel_state = POK_SYSTEM_STATE_OS_MOD; // TODO: is this choice for state right?

#ifdef POK_NEEDS_NETWORKING
   pok_network_init();
#endif

#ifdef POK_NEEDS_PARTITIONS
   pok_partition_arinc_init_all();
#endif
#ifdef POK_NEEDS_MONITOR
   pok_monitor_thread_init();
#endif
#ifdef POK_NEEDS_GDB
   pok_gdb_thread_init();
#endif

#if defined (POK_NEEDS_SCHED) || defined (POK_NEEDS_THREADS)
   pok_sched_init ();
#endif
#if defined (POK_NEEDS_PORTS_QUEUEING) || defined (POK_NEEDS_PORTS_SAMPLING)
   pok_channels_init_all ();
#endif

#if defined (POK_NEEDS_DEBUG) || defined (POK_NEEDS_CONSOLE)
  pok_cons_write ("JET OS kernel initialized\n", 26);
#endif

#ifdef POK_NEEDS_INSTRUMENTATION
  uint32_t tmp;
   printf ("[INSTRUMENTATION][CHEDDAR] <event_table>\n");
   printf ("[INSTRUMENTATION][CHEDDAR] <processor>\n");
   printf ("[INSTRUMENTATION][CHEDDAR] <name>pok_kernel</name>\n");

   for (tmp = 0 ; tmp < POK_CONFIG_NB_THREADS ; tmp++)
   {
      printf ("[INSTRUMENTATION][CHEDDAR] <task_activation>   0   task %lu</task_activation>\n", tmp);
   }
#endif


#ifdef POK_NEEDS_PARTITIONS
#if defined(POK_NEEDS_GDB) && defined(POK_NEEDS_WAIT_FOR_GDB)
  printf("Waiting for GDB connection ...\n");
  printf("\n");
  pok_trap();
#endif
  pok_sched_start();
#else
  ja_preempt_enable();

  /**
   * If we don't use partitioning service, we execute a main
   * function. In that case, POK is acting like an executive,
   * not a real kernel
   */
  main ();
#endif
}
