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
#define PRIME_RANGE_BEGIN 1
#define PRIME_RANGE_END   100000
#define SECOND 1000000000LL

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
    uint32_t period_32, duration_32;

    while(1)
    {
        /* PRIMES Iterations */
        count = 0;
        for(i = PRIME_RANGE_BEGIN; i < PRIME_RANGE_END; ++i)
        {
            if(i % ((PRIME_RANGE_END - PRIME_RANGE_BEGIN) / 30) == 0)
                printf("*");

            count += is_prime(i);
        }

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        period_32   = (uint32_t)pr_stat.PERIOD;
        duration_32 = (uint32_t)pr_stat.DURATION;

        printf("\n[p%d:%u|%u|%d] %u primes exist in [%u; %u]\n",
               pr_stat.IDENTIFIER, period_32, duration_32,
               pr_stat.OPERATING_MODE, count, PRIME_RANGE_BEGIN,
               PRIME_RANGE_END);

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

    tattr.ENTRY_POINT   = prime_thread_func;
    tattr.DEADLINE      = HARD;
    tattr.PERIOD        = 30000000000;
    tattr.STACK_SIZE    = 8096;
    tattr.TIME_CAPACITY = 30000000000;
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
