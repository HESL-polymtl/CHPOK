/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Matrix convolution
 *
 *
 ******************************************************************************/

#include "../../../../A653_Benchmarks/BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define MATRIX_SIZE 200
#define KERNEL_SIZE 5

float matrix[MATRIX_SIZE][MATRIX_SIZE];
float result[MATRIX_SIZE][MATRIX_SIZE];
float kernel[KERNEL_SIZE][KERNEL_SIZE] = {
    {1 / 256,  4 / 256,  6 / 256,  4 / 256, 1 / 256},
    {4 / 256, 16 / 256, 24 / 256, 16 / 256, 4 / 256},
    {6 / 256, 24 / 256, 36 / 256, 24 / 256, 6 / 256},
    {4 / 256, 16 / 256, 24 / 256, 16 / 256, 4 / 256},
    {1 / 256,  4 / 256,  6 / 256,  4 / 256, 1 / 256},
};

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

float get_matrix_val(int i, int j)
{
    if(i < 0 || j < 0 || i > MATRIX_SIZE || j > MATRIX_SIZE)
        return -1;

    return matrix[i][j];
}

void matconv()
{
    const int kernel_bound = KERNEL_SIZE / 2;



    for(unsigned int i = 0; i < MATRIX_SIZE; ++i)
    {
        for(unsigned int j = 0; j < MATRIX_SIZE; ++j)
        {
            matrix[i][j] = rand();
            result[i][j] = 0;
        }
    }
    for(unsigned int i = 0; i < MATRIX_SIZE; ++i)
    {
        for(unsigned int j = 0; j < MATRIX_SIZE; ++j)
        {
            unsigned int acc    = 0;

            for(int k = -kernel_bound; k < kernel_bound; ++k)
            {
                for(int l = -kernel_bound; l < kernel_bound; ++l)
                {
                    unsigned int val = get_matrix_val(i + k, j + l);
                    if(val != -1)
                    {
                        acc += val * kernel[kernel_bound + k][kernel_bound + l];
                    }
                }
            }

            result[i][j] = acc;
        }
    }
}

void* matconv_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[MATCONV] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Convolution\n");
        matconv();

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[MATCONV] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_matconv;
    PROCESS_ATTRIBUTE_TYPE th_attr_matconv;

    RETURN_CODE_TYPE       ret_type;


    /* Set MATCONV manipulation process */
    th_attr_matconv.ENTRY_POINT   = matconv_thread;
    th_attr_matconv.DEADLINE      = HARD;
    th_attr_matconv.PERIOD        = 1000000000;
    th_attr_matconv.STACK_SIZE    = 4096;
    th_attr_matconv.TIME_CAPACITY = 1000000000;
    th_attr_matconv.BASE_PRIORITY = 1;
    memcpy(th_attr_matconv.NAME, "MATCONV_A653\0", 13* sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_matconv, &thread_matconv, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create MATCONV process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_matconv, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start MATCONV process[%d]\n", ret_type);
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
