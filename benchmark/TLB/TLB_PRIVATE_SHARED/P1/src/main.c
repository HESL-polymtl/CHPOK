/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: TLB tests, Private DTLB and Shared TLB
 * Activity:
 *    Reading memory data
 *
 * This test aim to study the TLB interferences. Each access of the programm
 * will lead to a TLB miss. This allow us to determine the interferences and
 * delay induced by the use of the TLB. Both the private levels and the shared
 * levels of data TLB can be studied depending on the WALK_SIZE parameter.
 *
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

#define DATA_SIZE ((TLB_SIZE_SHARED + TLB_SIZE_DATA) * SYSTEM_PAGE_SIZE)


/* The walk size will be repeated infinitely, you can modify its value to
 * set the quantity of data to be walked, depending on the CLEAR_TLB
 * value, a small walk size will only produce TLB miss on the first walk
 */
#define WALK_SIZE DATA_SIZE

/* Define if at the end of each walk, the TLB must be cleared. Actually since
 * we want the tests to be portable and not all OS allow us to clear the TLB
 * we use two separate banks that are accessed one walk after the other.
 * If corretly set (and if this is the behaviour you want), the fist walk will
 * populate the entier TLB. The second walk will use the second bank and
 * always miss. The third walk will use the first banks that has been evicted
 * with the second walk, etc.
 *
 * When CLEAR_TLB is disabled, only one bank is used.
 */
#define CLEAR_TLB 1

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

static uint8_t data0[DATA_SIZE] __attribute__ ((aligned (SYSTEM_PAGE_SIZE)));
static uint8_t data1[DATA_SIZE] __attribute__ ((aligned (SYSTEM_PAGE_SIZE)));

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* tlb_flood(void)
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
            start_address += SYSTEM_PAGE_SIZE;
        }
        ++tmp;

#if CLEAR_TLB
        current_bank = (current_bank + 1) % 2;
#endif

        if(++k == 100)
        {
            printf("TLB flooded 100 times\n");
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
    PROCESS_ID_TYPE        thread_tlb_0;
    PROCESS_ATTRIBUTE_TYPE tattr_tlb_0;

    RETURN_CODE_TYPE       ret_type;


    /* Set TLB manipulation process */
    tattr_tlb_0.ENTRY_POINT   = tlb_flood;
    tattr_tlb_0.DEADLINE      = HARD;
    tattr_tlb_0.PERIOD        = 200000000;
    tattr_tlb_0.STACK_SIZE    = 2000;
    tattr_tlb_0.TIME_CAPACITY = 200000000;
    tattr_tlb_0.BASE_PRIORITY = 1;
    memcpy(tattr_tlb_0.NAME, "TLB0_A653\0", 11 * sizeof(char));

    printf("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&tattr_tlb_0, &thread_tlb_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create TLB0 process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_tlb_0, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start TLB0 process[%d]\n", ret_type);
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
