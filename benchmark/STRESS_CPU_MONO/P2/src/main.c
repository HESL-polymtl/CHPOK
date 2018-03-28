/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: CPU stress tests
 * Activity:
 *    Square root computation
 *    Prime counter
 *
 * These tests aim to use the CPU at its maximum capacity. Memory footprint of
 * these tests should be the lowest possible.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#define GET_STAT 1
#include "../../../BENCH_TOOLS/get_stat.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define SQRT_ITER_COUNT   100

static double sqrt(double value)
{
    double prev_val = value + 1;
    double next_val = value;

    while(prev_val != next_val)
    {
        prev_val = next_val;
        next_val = (prev_val + value / prev_val) / 2.0;
    }

    return next_val;
}

static void* sqrt_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    float sum;
    uint32_t i, j;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        /* SQRT Iterations */
        sum = 0;
        for(i = 1; i < SQRT_ITER_COUNT; ++i)
        {
            for(j = 1; j < SQRT_ITER_COUNT; ++j)
                sum += sqrt(i) * sqrt(j);

        }

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        printf("\n[p%ld:%lld|%lld|%d] SQRT Sum: %f\n", pr_stat.IDENTIFIER,
               pr_stat.PERIOD, pr_stat.DURATION, pr_stat.OPERATING_MODE, sum);

        GET_ET_STAT_3_3

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot achieve periodic wait\n");
            return (void*)1;
        }
    }

    return (void*)0;
}

/*******************************************************************************
 * TESTS MAIN
 ******************************************************************************/
int main ()
{
    PROCESS_ATTRIBUTE_TYPE tattr;
    PROCESS_ID_TYPE        thread;
    RETURN_CODE_TYPE       ret_type;

    tattr.ENTRY_POINT   = sqrt_thread_func;
    tattr.DEADLINE      = HARD;
    tattr.PERIOD        = 400000000;
    tattr.STACK_SIZE    = 8096;
    tattr.TIME_CAPACITY = 200000000;
    tattr.BASE_PRIORITY = MIN_PRIORITY_VALUE;
    memcpy(&tattr.NAME, "SQRT_A653\0", 10 * sizeof(char));

    printf("Init SQRT partition\n");

    CREATE_PROCESS (&tattr, &thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create SQRT process[%d]\n", ret_type);
    }

    START(thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start SQRT process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("SQRT partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch SQRT partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}
