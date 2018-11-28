/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: Cache Tests Bank: False Sharing tests
 * Activity:
 *    Reading/Writing Caches
 *
 * This tests aim to study the cache management by the OS. Multiple threads from
 * multiple partitions are using different cache lines and producing false
 * sharing behaviours. The WCET should not be impacted if the RTOS manages the
 * caches correctly.
 *
 * Cache Index : I = (V mod (S / N)) / L
 *     I: Cache line index;
 *     V: Virtual address;
 *     S: Cache size;
 *     N: Number of cache sets;
 *     L: Line size;
 *
 * V + n*(S/N) leads to the same cache index
 *
 * Protocol:
 *    10000 Execution each time.
 *    Start with a data set of 1/4 L1 cache size;
 *    End with a data set of 4 L3 cache size;
 *    1/4 L1 cache size increment
 *
 *
 * Possible measures:
 *    WCET/ACET/BCET
 *    Cache Miss/Hit
 *    Write to MEM
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#include "../../../../../BENCH_TOOLS/config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define DATA_SIZE (CACHE_SIZE_L3 / 2)

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

static uint8_t data[DATA_SIZE] = {0};

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* cache_thread_func_0(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;


    uint32_t inc_size = CACHE_SIZE_L1_DATA / 4;
    uint32_t start_size = 0;

    uint32_t nb_exec = 0;

    uint32_t start_address;
    uint32_t end_address;
    uint32_t inc_address = CACHE_SIZE_L1_DATA / CACHE_SETS_NUMBER_L1_DATA;

    uint32_t target_cache_line, current_target;

    uint32_t i, j;

    SYSTEM_TIME_TYPE wcet, acet, bcet, ex_time, start_time, end_time;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[TH0] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        if(nb_exec % 100 == 0)
        {
            /* Set the target cache line */
            target_cache_line = 0;

            /* Set the data range size */
            start_size += inc_size;
            if(start_size > DATA_SIZE)
            {
                printf("---- [p%ld:%lld|%lld|%d] Benchmark END",
                       pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                       pr_stat.OPERATING_MODE);

                STOP_SELF();
            }

            /* Compute the start address of the range */
            start_address = (uint32_t)data;
            current_target = (start_address %
                              (CACHE_SIZE_L1_DATA / CACHE_SETS_NUMBER_L1_DATA)) /
                             CACHE_LINE_SIZE_L1_DATA;

            while(target_cache_line != current_target)
            {
                start_address++;

                current_target = (start_address %
                                  (CACHE_SIZE_L1_DATA / CACHE_SETS_NUMBER_L1_DATA)) /
                                 CACHE_LINE_SIZE_L1_DATA;
            }

            /* Compute the usable address range */
            end_address = start_address + start_size;
            end_address = end_address > ((uint32_t)data + DATA_SIZE) ?
                                        ((uint32_t)data + DATA_SIZE) :
                                        end_address;

            if(nb_exec != 0)
            {
                printf("---- [p%ld:%lld|%lld|%d] End walk:\n\tWCET = %llu\n\
    \tACET = %llu\n\tBCET = %llu\n",
                       pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                       pr_stat.OPERATING_MODE,
                       wcet, acet, bcet);
            }

            printf("\n[p%ld:%lld|%lld|%d] Starting new walk:\n\tStart = 0x%08x\n\
\tEnd = 0x%08x\n\tStep = 0x%08x\n\tDataset size = %uB\n",
                   pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                   pr_stat.OPERATING_MODE,
                   start_address, end_address, inc_address, start_size);

            nb_exec = 1;
            wcet = 0;
            acet = 0;
            bcet = 0x7FFFFFFFFFFFFFFF;
        }
        else
        {
            GET_TIME(&start_time, &ret_type);

            /* Walk the memory */
            for(j = 0; j < 100; ++j)
            {
                for(i = start_address; i < end_address; i += inc_address * 2)
                {
                    *((uint8_t*)i) = *((uint8_t*)i) + 1;
                }
            }

            GET_TIME(&end_time, &ret_type);
            ex_time = end_time - start_time;

            if(ex_time > wcet)
                wcet = ex_time;

            if(ex_time < bcet)
                bcet = ex_time;

            acet = (acet * (nb_exec) + ex_time) / (nb_exec + 1);
        }

        ++nb_exec;

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[TH0] Cannot achieve periodic wait [%d]\n", ret_type);
            return (void*)1;
        }
    }

    return (void*)0;
}

/*******************************************************************************
 * TESTS MAIN
 ******************************************************************************/
int main()
{
    PROCESS_ID_TYPE        thread_cache_0;
    PROCESS_ATTRIBUTE_TYPE tattr_cache_0;

    RETURN_CODE_TYPE       ret_type;


    /* Set CACHE manipulation process */
    tattr_cache_0.ENTRY_POINT   = cache_thread_func_0;
    tattr_cache_0.DEADLINE      = HARD;
    tattr_cache_0.PERIOD        = 200000000;
    tattr_cache_0.STACK_SIZE    = 2000;
    tattr_cache_0.TIME_CAPACITY = 200000000;
    tattr_cache_0.BASE_PRIORITY = 1;
    memcpy(tattr_cache_0.NAME, "CACHE0_A653\0", 12 * sizeof(char));

    printf("Init P1 partition\n");

    /* Create processes */
    CREATE_PROCESS(&tattr_cache_0, &thread_cache_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create CACHE0 process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_cache_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start CACHE0 process[%d]\n", ret_type);
        return -1;
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("P1 partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch P1 partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}
