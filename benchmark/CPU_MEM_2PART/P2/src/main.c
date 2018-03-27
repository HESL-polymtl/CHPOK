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
#define MAT_SIZE_N 500
#define MAT_SIZE_M 500
#define NB_DIVISIONS 5

#define PROC_PERIOD     10000000000ll
#define DELAY_MAT_MULT   1000000000ll
#define PROC_DEADLINE   PROC_PERIOD

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

static uint32_t matA[MAT_SIZE_N][MAT_SIZE_M];
static uint32_t matB[MAT_SIZE_M][MAT_SIZE_N];
static uint32_t matC[MAT_SIZE_N][MAT_SIZE_N];
static uint32_t initialized_mult_mat_processes = 0;
static uint32_t partition_id;

//Used for random
static uint32_t lfsr = 0xABCDEF01;
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

static uint32_t my_rand()
{
    uint32_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    return lfsr =  (lfsr >> 1) | (bit << 15);
}

static void randomize_matrix(uint32_t n, uint32_t m, uint32_t *mat)
{
    for (uint32_t i = 0; i < n; ++i)
    {
        for (uint32_t j = 0; j < m; ++j)
        {
            *(mat + (i*n +j)) = my_rand();
        }
    }
    return;
}


void* generate_mat_func(void)
{
    RETURN_CODE_TYPE ret_type;

    printf("[Part %d] First pass of generate_mat function\n", partition_id);

    while(1)
    {
        printf("[Part %d] Generating matrix A and B\n", partition_id);

        randomize_matrix(MAT_SIZE_N,MAT_SIZE_M, (uint32_t*) matA);
        printf("[Part %d] Done generating matrix A\n", partition_id);
        
        randomize_matrix(MAT_SIZE_M,MAT_SIZE_N, (uint32_t*) matB);
        printf("[Part %d] Done generating matrixes A and B\n", partition_id);

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot achieve periodic wait\n", partition_id);
            return (void*)1;
        }
    }

    return (void*)0;
}

void* multiply_mat_func(void)
{
    RETURN_CODE_TYPE ret_type;
    
    /* MUTEX LOCK TODO */
    uint32_t my_index = initialized_mult_mat_processes++;
    /* MUTEX UNLOCK TODO*/

    printf("[Part %d] First pass of mult_mat function index: %d\n", 
            partition_id, my_index);

    uint32_t nb_rows = MAT_SIZE_N/NB_DIVISIONS;
    uint32_t beg_row = nb_rows*my_index;

    while(1)
    {


        for (uint32_t i = beg_row; i < beg_row + nb_rows; ++i)
        {
            for (uint32_t j = 0; j < MAT_SIZE_M; ++j)
            {
                uint32_t value = 0;
                for (uint32_t k = 0; k < MAT_SIZE_N; ++k)
                {
                    value += matA[i][k]*matB[k][j];
                }
                matC[i][j] = value;
            }
        }

        printf("[Part %d] Done multiplying rows %d to rows %d\n",
                partition_id, beg_row, beg_row + nb_rows - 1);

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot achieve periodic wait\n", partition_id);
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
    PARTITION_STATUS_TYPE part_stat;
    RETURN_CODE_TYPE       ret_type;   
    GET_PARTITION_STATUS (&part_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot get partition status\n");
        return 1;
    }
    partition_id = part_stat.IDENTIFIER;

    printf("[Part %d] Init MAT_MULTIPLY partition\n", partition_id);
    

    PROCESS_ID_TYPE        threads[1 + NB_DIVISIONS];
    PROCESS_ATTRIBUTE_TYPE tattr = {
        .ENTRY_POINT   = generate_mat_func,
        .DEADLINE      = HARD,
        .PERIOD        = PROC_PERIOD,
        .STACK_SIZE    = 8096,
        .TIME_CAPACITY = PROC_DEADLINE,
        .BASE_PRIORITY = 2,
        .NAME = "GENERATE_MAT_A653"
    };


    CREATE_PROCESS (&tattr, &threads[0], &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot create GENERATE_MAT process[%d]\n", 
                partition_id, ret_type);
    }
    printf("[Part %d] GENERATE_MAT thread number: %d\n", 
            partition_id, threads[0]);

    for (uint32_t i = 0; i < NB_DIVISIONS; ++i)
    {
        PROCESS_ATTRIBUTE_TYPE tattr = {
            .ENTRY_POINT   = multiply_mat_func,
            .DEADLINE      = HARD,
            .PERIOD        = PROC_PERIOD,
            .STACK_SIZE    = 8096,
            .TIME_CAPACITY = PROC_DEADLINE,
            .BASE_PRIORITY = 1,
            .NAME = "MAT_MULTIPLY_A653_"
        };
        char index_char[10];
        itoa(i,index_char,10);
        for(uint32_t k=0; k<strlen(index_char)+1; ++k){
            tattr.NAME[18+k] = index_char[k];
        }

        CREATE_PROCESS(&tattr, &threads[1+i], &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot create MAT_MULTIPLY_N%d process[%d]\n", 
                    partition_id, i, ret_type);
        }
        printf("[Part %d] ", partition_id);
        printf(tattr.NAME);
        printf(" thread number: %d\n", threads[1+i]);
    }

    START(threads[0], &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot start GENERATE_MAT process[%d]\n", 
                partition_id, ret_type);
    }

    for (uint32_t i = 1; i < NB_DIVISIONS + 1; ++i)
    {
        DELAYED_START(threads[i], DELAY_MAT_MULT, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[Part %d] Cannot start MAT_MULTIPLY_N%d process[%d]\n", 
                    partition_id, i, ret_type);
        }
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("[Part %d] MAT_MULT partition switching to normal mode\n",
            partition_id);
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[Part %d] Cannot switch MAT_MULT partition to NORMAL state[%d]\n", 
                partition_id, ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}
