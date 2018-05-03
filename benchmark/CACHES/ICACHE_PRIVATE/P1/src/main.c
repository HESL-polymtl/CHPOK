/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: Cache Tests Bank: Private Intruction cache stress
 * Activity:
 *    Reading/Writing Caches
 *
 * This tests aim to study the cache management by the OS. Multiple threads from
 * multiple partitions are filling the caches gradually. The effect of cache
 * hit / miss and eviction policy will be studied tahnks to these tests.
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
 *    Start with a data set of 1/10 L1 cache size;
 *    End with a data set of 4 L3 cache size;
 *    1/10 L1 cache size increment
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

#include "../../../../BENCH_TOOLS/config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define ARCH_X86 1
#define ARCH_PPC 0
#define ARCH_ARM 0

#if ARCH_X86
    #include "icache_flood_x86.h"
#elif ARCH_PPC
    #include "icache_flood_ppc.h"
#elif ARCH_ARM
    #include "icache_flood_arm.h"
#endif

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* flood_th(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t k;
    k = 0;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[TH0] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
#if ARCH_X86 || ARCH_PPC || ARCH_ARM
        asm_cache_flood();
#else
        printf("Error: architecture not defined or not supported\n");
#endif

        if(++k == 1000)
        {
            printf("L1 Cache filled 1000 times\n");
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
    PROCESS_ID_TYPE        thread_flood;
    PROCESS_ATTRIBUTE_TYPE th_attr_flood;

    RETURN_CODE_TYPE       ret_type;

    /* Set flood manipulation process */
    th_attr_flood.ENTRY_POINT   = flood_th;
    th_attr_flood.DEADLINE      = HARD;
    th_attr_flood.PERIOD        = 50000000;
    th_attr_flood.STACK_SIZE    = 1000;
    th_attr_flood.TIME_CAPACITY = 50000000;
    th_attr_flood.BASE_PRIORITY = 1;
    memcpy(th_attr_flood.NAME, "FLOOD_A653\0", 11 * sizeof(char));

    printf("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_flood, &thread_flood, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create FLOOD process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_flood, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start FLOOD process[%d]\n", ret_type);
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
