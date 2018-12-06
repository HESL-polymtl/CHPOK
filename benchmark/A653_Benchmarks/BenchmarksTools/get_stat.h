/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Felipe Gohring de Magalhaes
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Global statistics tools and instrumentation.
 ******************************************************************************/

#ifndef __GET_STAT_H_
#define __GET_STAT_H_

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>
#include <arinc653/process.h>
#include "benc_config.h"

#if GET_STAT
    #define GET_ET_STAT_1_3                                                    \
        SYSTEM_TIME_TYPE    WCET_ET_STAT = 0;                                  \
        SYSTEM_TIME_TYPE    ACET_ET_STAT = 0;                                  \
        SYSTEM_TIME_TYPE    BCET_ET_STAT = 0x7FFFFFFFFFFFFFFF;                 \
        SYSTEM_TIME_TYPE    START_ET_STAT, END_ET_STAT;                        \
        uint64_t            EX_ET_STAT = 0;                                    \
        RETURN_CODE_TYPE    RET_TYPE_ET_STAT;                                  \
        PROCESS_STATUS_TYPE PROCESS_STATUS_ET_STAT;                            \
        PROCESS_ID_TYPE     PROCESS_ID_ET_STAT;                                \
                                                                               \
        GET_MY_ID(&PROCESS_ID_ET_STAT, &RET_TYPE_ET_STAT);                     \
        GET_PROCESS_STATUS(PROCESS_ID_ET_STAT, &PROCESS_STATUS_ET_STAT,        \
                           &RET_TYPE_ET_STAT);                                 \

    #define GET_ET_STAT_2_3 GET_TIME(&START_ET_STAT, &RET_TYPE_ET_STAT);

    #define GET_ET_STAT_3_3                                                    \
        GET_TIME(&END_ET_STAT, &RET_TYPE_ET_STAT);                             \
                                                                               \
        SYSTEM_TIME_TYPE DIFF_ET_STAT = END_ET_STAT - START_ET_STAT;           \
        WCET_ET_STAT = WCET_ET_STAT < DIFF_ET_STAT ?                           \
                       DIFF_ET_STAT : WCET_ET_STAT;                            \
        BCET_ET_STAT = BCET_ET_STAT > DIFF_ET_STAT ?                           \
                       DIFF_ET_STAT : BCET_ET_STAT;                            \
        ACET_ET_STAT = (ACET_ET_STAT * (EX_ET_STAT) + DIFF_ET_STAT) /          \
                       (EX_ET_STAT + 1);                                       \
        ++EX_ET_STAT;                                                          \
        OUTPUT("[%s] ET %ums, WCET %ums, BCET %ums, ACET %ums, EXE %u\n",      \
               PROCESS_STATUS_ET_STAT.ATTRIBUTES.NAME,                         \
               (uint32_t)(DIFF_ET_STAT/1000000),                               \
               (uint32_t)(WCET_ET_STAT/1000000),                               \
               (uint32_t)(BCET_ET_STAT/1000000),                               \
               (uint32_t)(ACET_ET_STAT/1000000),                               \
               (uint32_t)EX_ET_STAT);
#else
    #define GET_ET_STAT_1_3
    #define GET_ET_STAT_2_3
    #define GET_ET_STAT_3_3
#endif

#endif /* __GET_STAT_H_ */
