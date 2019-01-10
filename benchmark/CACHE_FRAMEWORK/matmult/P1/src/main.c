/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Matrix multiplication
 *
 *
 ******************************************************************************/

#include "../../../../A653_Benchmarks/BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define MATRIX_SIZE 200

unsigned int matA[MATRIX_SIZE][MATRIX_SIZE];
unsigned int matB[MATRIX_SIZE][MATRIX_SIZE];
unsigned int matC[MATRIX_SIZE][MATRIX_SIZE];

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/


void matmult()
{
    for(unsigned int i = 0; i < MATRIX_SIZE; ++i)
    {
        for(unsigned int j = 0; j < MATRIX_SIZE; ++j)
        {
            matC[i][j] = 0;
        }
    }
    for(unsigned int i = 0; i < MATRIX_SIZE; ++i)
    {
        for(unsigned int j = 0; j < MATRIX_SIZE; ++j)
        {
            for(unsigned int k = 0; k < MATRIX_SIZE; ++k)
            {
                matC[i][k] += matA[i][k] * matB[k][j];
            }
        }
    }
}

void* matmult_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[MATMULT] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Multiplication\n");
        matmult();

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[MATMULT] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_matmult;
    PROCESS_ATTRIBUTE_TYPE th_attr_matmult;

    RETURN_CODE_TYPE       ret_type;


    /* Set MATMULT manipulation process */
    th_attr_matmult.ENTRY_POINT   = matmult_thread;
    th_attr_matmult.DEADLINE      = HARD;
    th_attr_matmult.PERIOD        = 1000000000;
    th_attr_matmult.STACK_SIZE    = 4096;
    th_attr_matmult.TIME_CAPACITY = 1000000000;
    th_attr_matmult.BASE_PRIORITY = 1;
    memcpy(th_attr_matmult.NAME, "MATMULT_A653\0", 13* sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_matmult, &thread_matmult, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create MATMULT process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_matmult, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start MATMULT process[%d]\n", ret_type);
        return -1;
    }

    /* Parition has been initialized, now switch to normal mode */
    OUTPUT("P0 partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot switch P0 partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}
