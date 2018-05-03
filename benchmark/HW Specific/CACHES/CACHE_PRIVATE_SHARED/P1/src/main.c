/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: Cache tests, Shared / Private caches tests
 * Activity:
 *    Reading/Writing Caches
 *
 * This test aims to study the cache management by the OS. The effect of cache
 * hit / miss and eviction policy will be studied thanks to these tests.
 * The application fill the cache depending on the selected settings.
 *
 * The private cache can be studied when the size of the walk is less or equals
 * to the size of the private caches.
 *
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

#define DATA_SIZE (CACHE_SIZE_L3)

/* The walk size will be repeated infinitely, you can modify its value to
 * set the quantity of data to be walked, depending on the CLEAR_CACHE
 * value, a small walk size will only produce cache miss on the first walk
 */
#define WALK_SIZE DATA_SIZE

/* Set this parameter to 1 if you want the cache to be read only.
 * When set to 0, the memory will be loaded to the cache and the ligne written
 * this will allow to cache to mark it as dirty and write it back to the mem
 * on cache flush.
 */
#define CACHE_RDONLY 1

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

static uint8_t data[DATA_SIZE];

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* cache_thread_func_0(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint8_t* start_address;
    uint8_t* end_address;

#if CACHE_RDONLY
    uint8_t tmp;
#endif

    uint32_t k;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[TH0] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    k = 0;

    while(1)
    {
        start_address = data;
        end_address =  MIN((data + DATA_SIZE), (data + WALK_SIZE))

        /* Walk the memory */
        while(start_address < end_address)
        {
#if CACHE_RDONLY
            tmp = *start_address;
#else
            *start_address = start_address;
#endif
            ++start_address;
        }

#if CACHE_RDONLY
        /* Avoid compiler warning */
        ++tmp;
#endif
        if(++k == 100)
        {
            printf("Cache flooded 100 times\n");
            k = 0;
        }

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

    printf("Init P0 partition\n");

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
    printf("P0 partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch P0 partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}
