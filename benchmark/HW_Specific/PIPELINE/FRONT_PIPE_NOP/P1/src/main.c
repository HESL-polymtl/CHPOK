/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: CPU pipeline test suite: Pipeline front-end
 * Activity:
 *    NOP
 *
 * This tests aim to study the effect of the pipeline stressing on the system.
 * The use of NOP will stress the front end of the CPU pipeline, allowing us to
 * study the effect of such a technique on other applications.
 *
 * The test will run on any architecture having NOP as a part of its instruction
 * set.
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

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* nop_th(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i, k;
    k = 0;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[TH0] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        for(i = 0; i < 10000; ++i)
        {
#if ARCH_X86 || ARCH_PPC || ARCH_ARM
            /* 300 NOP */
            __asm__ __volatile__(
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n" "NOP\n"
                    "NOP\n" "NOP\n" "NOP\n"
            );

#else
            printf("Error: architecture not defined or not supported\n");
#endif
        }
        if(++k == 1000)
        {
            printf("3000000 NOP DONE\n");
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
    PROCESS_ID_TYPE        thread_nop;
    PROCESS_ATTRIBUTE_TYPE th_attr_nop;

    RETURN_CODE_TYPE       ret_type;


    /* Set NOP manipulation process */
    th_attr_nop.ENTRY_POINT   = nop_th;
    th_attr_nop.DEADLINE      = HARD;
    th_attr_nop.PERIOD        = 10000000;
    th_attr_nop.STACK_SIZE    = 1000;
    th_attr_nop.TIME_CAPACITY = 10000000;
    th_attr_nop.BASE_PRIORITY = 1;
    memcpy(th_attr_nop.NAME, "NOP_A653\0", 9 * sizeof(char));

    printf("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_nop, &thread_nop, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create NOP process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_nop, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start NOP process[%d]\n", ret_type);
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
