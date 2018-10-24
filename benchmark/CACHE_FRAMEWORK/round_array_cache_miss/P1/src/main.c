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

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define WAY_SIZE 4096
#define LINE_SIZE 32

#define NB_PROCESSES 5

#define PROC_PERIOD     100000000ll
#define PROC_DEADLINE   PROC_PERIOD

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/
static volatile uint8_t __attribute__ ((aligned (LINE_SIZE))) tabs[NB_PROCESSES*WAY_SIZE] = {{0}};

static uint32_t partition_id;

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

char *itoa(int value, char *buff, int radix)
{
  char temp[16];
  int i = 0;
  int j = 0;
  unsigned int digit;
  unsigned int uvalue;

  if ((radix == 10) && (value < 0))
  {
    uvalue = -value;
    buff[j++] = '-';
  }
  else
    uvalue = value;

  do
  {
    digit = uvalue % radix;
    uvalue = (uvalue - digit)/radix;
    if (digit < 10)
      temp[i++] = (char)('0' + digit);
    else
      temp[i++] = (char)('a' + (digit - 10));
  }
  while (uvalue != 0);
  --i;
  do
    buff[j++] = temp[i--];
  while (i >= 0);
  buff[j] = '\0';
  return buff;
}

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
        register long id = my_id - 2;
        for(register int j = 0; j < NB_PROCESSES; ++j)
        {
            for(register int i=0; i < WAY_SIZE; i += LINE_SIZE)
            {
                tabs[j*WAY_SIZE + i]++;
            }
        }
        printf(".", id);    
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
    

    PROCESS_ID_TYPE        threads[NB_PROCESSES];

    for (uint32_t i = 0; i < NB_PROCESSES; ++i)
    {
        PROCESS_ATTRIBUTE_TYPE tattr = {
            .ENTRY_POINT   = process_func,
            .DEADLINE      = HARD,
            .PERIOD        = PROC_PERIOD,
            .STACK_SIZE    = 8096,
            .TIME_CAPACITY = PROC_DEADLINE,
            .BASE_PRIORITY = i+1,
            .NAME = "ROUND_ARRAY_A653_PROC_"
        };
        char index_char[10];
        itoa(i,index_char,10);
        for(uint32_t k=0; k<strlen(index_char)+1; ++k){
            tattr.NAME[22+k] = index_char[k];
        }

        CREATE_PROCESS(&tattr, &threads[i], &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot create ROUND_ARRAY_A653_PROC_%d process[%d]\n", 
                    partition_id, i, ret_type);
        }
        printf("[Part %d] ", partition_id);
        printf(tattr.NAME);
        printf(" thread number: %ld\n", threads[i]);
    }

    for (uint32_t i = 0; i < 1; ++i)
    {
        START(threads[i], &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot start ROUND_ARRAY_A653_PROC_%d process[%d]\n", 
                    partition_id, i, ret_type);
        }
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
