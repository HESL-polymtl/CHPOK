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

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define SQRT_ITER_COUNT   1000
#define SECOND 1000000000LL

/*******************************************************************************
 * TESTS FUNCTIONS
 * ARINC-653 Multicore RTOS Benchmark#define SECOND 1000000000LL
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
    uint32_t period_32, duration_32;
 *
 ******************************************************************************/

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
    uint32_t period_32, duration_32;

    while(1)
    {

        /* SQRT Iterations */
        sum = 0;
        for(i = 1; i < SQRT_ITER_COUNT; ++i)
        {
            for(j = 1; j < SQRT_ITER_COUNT; ++j)
                sum += sqrt(i) * sqrt(j);

            if(i % (SQRT_ITER_COUNT / 30) == 0)
                printf(".");
        }

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        period_32   = (uint32_t)pr_stat.PERIOD;
        duration_32 = (uint32_t)pr_stat.DURATION;

        printf("\n[p%d:%u|%u|%d] SQRT Sum: %f\n", pr_stat.IDENTIFIER,
               period_32, duration_32, pr_stat.OPERATING_MODE, sum);

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
    tattr.PERIOD        = 30000000000;
    tattr.STACK_SIZE    = 8096;
    tattr.TIME_CAPACITY = 30000000000;
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
