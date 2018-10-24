/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: CPU stress + Memory usage + IO Access
 * Activity:
 *    GPS position generator
 *    Speed and direction computation (+ log)
 *    Altitude and pression computation (+ log)
 *
 * The aim of the test is to share the resources between partitions.
 * We excpect these tests to allow us to analyse the memory access management
 * the RTOS implemented and how it reacts with memory and IO acces
 * intensive applications.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <libm.h>
#include <types.h>
#include <float.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>
#include <arinc653/semaphore.h>
#include <arinc653/sampling.h>

#include "../../config.h"

//#define GET_STAT 1
#include "../../../BENCH_TOOLS/get_stat.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define GENERATOR_LOAD 3000  /* Processing simulation */
#define MAT_SIZE 20

//#define VERBOSE_OUTPUT_PROC
#define OUTPUT_PROC

#ifndef OUTPUT_PROC
#define printf(fmt, ...) ;
#endif

/*******************************************************************************
 * TESTS VARS
 ******************************************************************************/

#define ABS(x) ((x) < 0 ? -(x) : (x))

static uint8_t mat[MAT_SIZE * MAT_SIZE];
static uint8_t image[MAT_SIZE * MAT_SIZE];


/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

static inline uint8_t getVal(int index, int xDiff, int yDiff, int img_width, uint8_t * Y) 
{ 
	return Y[index + (yDiff * img_width) + xDiff]; 
};

uint8_t sobel_operator(const int index, const int imgWidth, uint8_t * image)
{
	int x_weight = 0;
	int y_weight = 0;

	unsigned edge_weight;
	uint8_t edge_val;

	const char x_op[3][3] = {	{ -1,0,1 },
								{ -2,0,2 },
								{ -1,0,1 } };

	const char y_op[3][3] = {	{ 1,2,1 },
								{ 0,0,0 },
								{ -1,-2,-1 } };

	//Compute approximation of the gradients in the X-Y direction
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
            // X direction gradient
            x_weight = x_weight + (getVal(index, i - 1, j - 1, imgWidth, image) * x_op[i][j]);

            // Y direction gradient
            y_weight = y_weight + (getVal(index, i - 1, j - 1, imgWidth, image) * y_op[i][j]);
		}
	}

	edge_weight = ABS(x_weight) + ABS(y_weight);

	edge_val = (255 - (uint8_t)(edge_weight));

	//Edge thresholding
	if (edge_val > 200)
		edge_val = 255;
	else if (edge_val < 100)
		edge_val = 0;

	return edge_val;
}

void* pro_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
             printf("[PRO] ERROR Cannot get partition status\n");
             return (void*)1;
        }

        /* Computation simulation */
        for (unsigned int i = 0; i < MAT_SIZE; ++i) {
            mat[i*MAT_SIZE] = 0;
            mat[(i + 1)*MAT_SIZE - 1] = 0;
        }
        for (unsigned int j = 1; j < MAT_SIZE - 1; ++j) {
            mat[j] = 0;
            mat[(MAT_SIZE - 1)*MAT_SIZE + j] = 0;
        }

        /*Filtrage Sobel de l'image*/
        unsigned int index;
        for (unsigned int i = 1; i < MAT_SIZE - 1; ++i)
        {
            for (unsigned int j = 1; j < MAT_SIZE - 1; ++j) {
                index = i*MAT_SIZE + j;
                mat[index] = sobel_operator(index, MAT_SIZE, image);
            }
        }

        printf("Sobel computed\n");
        

        GET_ET_STAT_3_3

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[PRO] ERROR Cannot achieve periodic wait\n");
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
    PROCESS_ATTRIBUTE_TYPE tattr_sob;
    PROCESS_ID_TYPE        thread_sob;
    RETURN_CODE_TYPE       ret_type;

    printf("Init SOBE partition\n");

    /* Set spdi computation process */
    tattr_sob.ENTRY_POINT   = pro_thread_func;
    tattr_sob.DEADLINE      = HARD;
    tattr_sob.PERIOD        = GPS_PERIOD;
    tattr_sob.STACK_SIZE    = 2000;
    tattr_sob.TIME_CAPACITY = 500000000;
    tattr_sob.BASE_PRIORITY = 1;

    memcpy(&tattr_sob.NAME, "SOBE_PRO_A653\0", 14 * sizeof(char));


    CREATE_PROCESS (&tattr_sob, &thread_sob, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create SOBEL PROCESSING process[%d]\n", ret_type);
    }

    /* Start all processes */

    START(thread_sob, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start SOBEL PROCESSING process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("PROC partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch PROC partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}

#ifdef OUTPUT_PROC
#undef printf
#endif
