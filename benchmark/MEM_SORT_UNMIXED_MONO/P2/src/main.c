/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: Memory intenssive tests
 * Activity:
 *    Random number generators
 *    Sorters
 *
 * These tests aim to use the memory as much as possible to make it the
 * perfomances bottleneck.
 * We excpect these tests to allow us to analyse the memory access management
 * the RTOS implemented and how it reacts with memory intenssive applications.
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
#define GENERATOR_LOAD 5000 /* Amount of number to generate per pertiod */

 /*******************************************************************************
  * TESTS VARS
  ******************************************************************************/
static uint32_t to_sort[GENERATOR_LOAD];

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* sort_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i, j;
    uint32_t swap, swap_count;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        swap_count = 1;
        /* Number generation */
        for(j = 0; j < GENERATOR_LOAD && swap_count != 0; ++j)
        {
            swap_count = 0;

            for(i = 0; i < GENERATOR_LOAD - 1; ++i)
            {
                if(to_sort[i] > to_sort[i + 1])
                {
                    swap = to_sort[i];
                    to_sort[i] = to_sort[i + 1];
                    to_sort[i + 1] = swap;
                    ++swap_count;
                }
            }
        }

        for(i = 0; i < GENERATOR_LOAD - 1; ++i)
        {
            if(to_sort[i] > to_sort[i+1])
            {
                printf("ERROR: Array is not sorted!");
                break;
            }
        }

        printf("[p%ld:%lld|%lld|%d] BUBBLE SORTED, WAITING\n",
               pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
               pr_stat.OPERATING_MODE);

        GET_ET_STAT_3_3

        printf("\n");

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot achieve periodic wait\n");
            return (void*)1;
        }
    }

    return (void*)0;
}

void* gen_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }

        /* Number generation */
        for(i = 0; i < GENERATOR_LOAD; ++i)
        {
            to_sort[i] = GENERATOR_LOAD - i;
        }

        printf("[p%ld:%lld|%lld|%d] GENERATED, WAITING\n",
               pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
               pr_stat.OPERATING_MODE);

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
    PROCESS_ATTRIBUTE_TYPE tattr_gen;
    PROCESS_ATTRIBUTE_TYPE tattr_sort;
    PROCESS_ID_TYPE        thread_gen;
    PROCESS_ID_TYPE        thread_sort;
    RETURN_CODE_TYPE       ret_type;

    printf("Init SORT2 partition\n");

    /* Set GENERATOR process */
    tattr_gen.ENTRY_POINT   = gen_thread_func;
    tattr_gen.DEADLINE      = HARD;
    tattr_gen.PERIOD        = 4700000000;
    tattr_gen.STACK_SIZE    = 2000;
    tattr_gen.TIME_CAPACITY = 10000000;
    tattr_gen.BASE_PRIORITY = 1;
    memcpy(&tattr_gen.NAME, "GEN_A653\0", 9 * sizeof(char));

    /* Set SORTER process */
    tattr_sort.ENTRY_POINT   = sort_thread_func;
    tattr_sort.DEADLINE      = HARD;
    tattr_sort.PERIOD        = 4700000000;
    tattr_sort.STACK_SIZE    = 2000;
    tattr_sort.TIME_CAPACITY = 1190000000;
    tattr_sort.BASE_PRIORITY = 1;
    memcpy(&tattr_sort.NAME, "SORT_A653\0", 10 * sizeof(char));

    /* Create processes */
    CREATE_PROCESS (&tattr_gen, &thread_gen, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create GENEATOR process[%d]\n", ret_type);
    }
    CREATE_PROCESS (&tattr_sort, &thread_sort, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create SORT process[%d]\n", ret_type);
    }

    /* Start all processes */
    START(thread_gen, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start GENERATOR process[%d]\n", ret_type);
    }
    START(thread_sort, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start SORT process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("SORT2 partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch SORT2 partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}
