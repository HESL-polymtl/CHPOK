/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: Memory: Memory stress tests
 * Activity:
 *    Reading/Writing Main memory
 *
 * This test aims to access the main memory only, avoiding the use of the
 * caches. We expect all the accesses to produce a cache miss at the LLC.
 * This behaviour will allow to study the effect of the memory and bus
 * contention in the system.
 * Going further, we could use these benchmarks to study the contention on the
 * bus is the memory contention effect can be substracted to our results.
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
 *
 * Possible measures:
 *    WCET/ACET/BCET
 *    Cache Miss/Hit
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

 #define MIN(v1, v2) (v1 < v2 ? v1 : v2)

/* I5 750 Intel processor:
 * Private L1 and L2
 */
#define CACHE_SIZE_L1 32768
#define CACHE_SIZE_L2 262144
#define CACHE_SIZE_L3 8388608

#define CACHE_LINE_SIZE_L1 64
#define CACHE_LINE_SIZE_L2 64
#define CACHE_LINE_SIZE_L3 64

#define CACHE_ASSOC_NUMBER_L1 8
#define CACHE_ASSOC_NUMBER_L2 8
#define CACHE_ASSOC_NUMBER_L3 16

#define CACHE_SETS_NUMBER_L1 (CACHE_SIZE_L1 / (CACHE_ASSOC_NUMBER_L1 * CACHE_LINE_SIZE_L1))
#define CACHE_SETS_NUMBER_L2 (CACHE_SIZE_L2 / (CACHE_ASSOC_NUMBER_L2 * CACHE_LINE_SIZE_L2))
#define CACHE_SETS_NUMBER_L3 (CACHE_SIZE_L3 / (CACHE_ASSOC_NUMBER_L3 * CACHE_LINE_SIZE_L3))

#define DATA_SIZE (CACHE_SIZE_L3 / 2)


/* The walk size will be repeated infinitely, you can modify its value to
 * set the quantity of data to be walked, depending on the CLEAR_CACHE
 * value, a small walk size will only produce cache miss on the first walk
 */
 #define WALK_SIZE DATA_SIZE

 /* Define if at the end of each walk, the cache must be cleared. Actually since
  * we want the tests to be portable and not all OS allow us to clear the caches
  * we use two separate banks that are accessed one walk after the other.
  * If corretly set (and if this is the behaviour you want), the fist walk will
  * populate the entier cache. The second walk will use the second bank and
  * always miss. The third walk will use the first banks that has been evicted
  * with the second walk, etc.
  *
  * When CLEAR_CACHE is disabled, only one bank is used.
  */
 #define CLEAR_CACHE 1

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

static uint8_t data0[DATA_SIZE];
static uint8_t data1[DATA_SIZE];

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* mem_thread_func_0(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint8_t* end_address;
    uint8_t* start_address;

    uint8_t tmp;

    uint8_t current_bank = 0;
    uint8_t k = 0;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[TH0] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        /* Get the current bank start address and end address */
        if(current_bank == 0)
        {
            start_address = data0;
            end_address =  MIN((data0 + DATA_SIZE), (data0 + WALK_SIZE));
        }
        else
        {
            start_address = data1;
            end_address =  MIN((data1 + DATA_SIZE), (data1 + WALK_SIZE));
        }

        /* Walk the memory */
        while(start_address < end_address)
        {
            tmp = *start_address;
            start_address += CACHE_LINE_SIZE_L3;
        }
        ++tmp;

#if CLEAR_CACHE
        current_bank = (current_bank + 1) % 2;
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
    PROCESS_ID_TYPE        thread_mem_0;
    PROCESS_ATTRIBUTE_TYPE tattr_mem_0;

    RETURN_CODE_TYPE       ret_type;


    /* Set MEM manipulation process */
    tattr_mem_0.ENTRY_POINT   = mem_thread_func_0;
    tattr_mem_0.DEADLINE      = HARD;
    tattr_mem_0.PERIOD        = 200000000;
    tattr_mem_0.STACK_SIZE    = 2000;
    tattr_mem_0.TIME_CAPACITY = 200000000;
    tattr_mem_0.BASE_PRIORITY = 1;
    memcpy(tattr_mem_0.NAME, "MEM_A653\0", 9 * sizeof(char));

    printf("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&tattr_mem_0, &thread_mem_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create MEM process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_mem_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start MEM process[%d]\n", ret_type);
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
