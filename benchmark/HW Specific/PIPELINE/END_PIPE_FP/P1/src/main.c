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
 * These test also allow to stress the end pipeline, which contains shared FPU
 * etc...
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#define GET_STAT 1
#include "../../../../BENCH_TOOLS/get_stat.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define PRIME_RANGE_BEGIN 1
#define PRIME_RANGE_END   10000

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/
static uint8_t is_prime(uint32_t value)
{
    uint32_t i;

    if(value == 0)
        return 0;

    for(i = 2; i < value; ++i)
        if(value % i == 0)
            return 0;

    return 1;
}

static void* prime_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i;
    uint32_t count;

    /* Probing point */
    GET_ET_STAT_1_3

    while(1)
    {
        /* Probing point */
        GET_ET_STAT_2_3

        /* PRIMES Iterations */
        count = 0;
        for(i = PRIME_RANGE_BEGIN; i < PRIME_RANGE_END; ++i)
        {
            count += is_prime(i);
        }

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        printf("\n[p%ld:%lld|%lld|%d] %u primes exist in [%u; %u]\n",
               pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
               pr_stat.OPERATING_MODE, count, PRIME_RANGE_BEGIN,
               PRIME_RANGE_END);

        /* Probing point */
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
 *************************095*****************************************************/
int main ()
{
    PROCESS_ATTRIBUTE_TYPE tattr;
    PROCESS_ID_TYPE        thread;
    RETURN_CODE_TYPE       ret_type;

    tattr.ENTRY_POINT   = prime_thread_func;
    tattr.DEADLINE      = HARD;
    tattr.PERIOD        = 400000000;
    tattr.STACK_SIZE    = 8096;
    tattr.TIME_CAPACITY = 200000000;
    tattr.BASE_PRIORITY = 50;
    memcpy(&tattr.NAME, "PRIME_A653\0", 11 * sizeof(char));

    printf("Init PRIME partition\n");

    CREATE_PROCESS (&tattr, &thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create PRIME process[%d]\n", ret_type);
    }

    START(thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start PRIME process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("PRIME partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch PRIME partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}
