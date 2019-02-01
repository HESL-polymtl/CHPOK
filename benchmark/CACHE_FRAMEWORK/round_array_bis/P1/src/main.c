/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team
 *
 * Created by Jean-Baptiste on 22/03/2018
 */

/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: CPU stress tests
 * Activity:
 *    Generating matrices
 *    Multiplying matrices
 *
 * This tests aim to use the CPU and Memory at a balanced rate
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#include "access_array.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define NB_SETS 128
#define NB_WAYS 5
#define LINE_SIZE 32

#define NB_LOOP 1

#define PROC_PERIOD     100000000ll
#define PROC_DEADLINE   PROC_PERIOD



/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/
static volatile uint32_t __attribute__ ((aligned (LINE_SIZE))) tabs[NB_SETS*NB_WAYS*LINE_SIZE/4] = {0};

static uint32_t partition_id;

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void* process_func(void)
{
    RETURN_CODE_TYPE ret;
    while(1)
    {
        long my_id;
        GET_MY_ID(&my_id, &ret);
        if(ret != NO_ERROR)
        {
            printf("[Part %d] Cannot get ROUND_ARRAY_A653_PROC_ id[%d]\n", 
                    partition_id, ret);
            STOP_SELF();
        }
        __asm__ __volatile__(".byte 0x0F, 0xA7, 0x00, 0x00\n");
        for(register int j = 0; j < NB_LOOP; ++j)
        {
            access_array(tabs);
        }
        __asm__ __volatile__(".byte 0x0F, 0xA6, 0x00, 0x00\n");
        printf("Finished loop\n");
        PERIODIC_WAIT(&ret);
    }
}

/*******************************************************************************
 * TESTS MAIN
 ******************************************************************************/
int main ()
{
    PARTITION_STATUS_TYPE part_stat;
    RETURN_CODE_TYPE       ret_type;   
    GET_PARTITION_STATUS (&part_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot get partition status\n");
        return 1;
    }
    partition_id = part_stat.IDENTIFIER;

    printf("[Part %d] Init ROUND_ARRAY_A653 partition\n", partition_id);
    

    PROCESS_ID_TYPE        thread;


    PROCESS_ATTRIBUTE_TYPE tattr = {
        .ENTRY_POINT   = process_func,
        .DEADLINE      = HARD,
        .PERIOD        = PROC_PERIOD,
        .STACK_SIZE    = 8096,
        .TIME_CAPACITY = PROC_DEADLINE,
        .BASE_PRIORITY = 1,
        .NAME = "ROUND_ARRAY_A653_PROC_0"
    };

    CREATE_PROCESS(&tattr, &thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot create ROUND_ARRAY_A653_PROC_0 process[%d]\n", 
                partition_id, ret_type);
    }
    printf("[Part %d] ", partition_id);
    printf(tattr.NAME);
    printf(" thread number: %ld\n", thread);


    START(thread, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot start ROUND_ARRAY_A653_PROC_0 process[%d]\n", 
                partition_id, ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("[Part %d] ROUND_ARRAY_A653 partition switching to normal mode\n",
            partition_id);
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot switch ROUND_ARRAY_A653 partition to NORMAL state[%d]\n", 
                partition_id, ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}
