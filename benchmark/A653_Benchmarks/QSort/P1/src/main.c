/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test:  Non-recursive version of quick sort algorithm.
 *        This example sorts n floating point numbers, arr[].
 *
 *
 * Ported from:
 *     SNU-RT Benchmarks (http://www.cprover.org/goto-cc/examples/snu.html)
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#include "../../../BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define ARRAY_SIZE 2000
#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;
#define M 7
#define NSTACK 100

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

float arr[ARRAY_SIZE];
int istack[NSTACK];

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

void sort(unsigned long n)
{
	unsigned long i,ir=n,j,k,l=0;
	int jstack=0;
	float a,temp;

	for (;;) {
		if (ir-l < M) {
			for (j=l+1;j<=ir;j++) {
				a=arr[j];
				for (i=j-1;i>=l;i--) {
					if (arr[i] <= a) break;
					arr[i+1]=arr[i];
				}
				arr[i+1]=a;
			}
			if (jstack == 0) break;
			ir=istack[jstack--];
			l=istack[jstack--];
		} else {
			k=(l+ir) >> 1;
			SWAP(arr[k],arr[l+1])
			if (arr[l] > arr[ir]) {
				SWAP(arr[l],arr[ir])
			}
			if (arr[l+1] > arr[ir]) {
				SWAP(arr[l+1],arr[ir])
			}
			if (arr[l] > arr[l+1]) {
				SWAP(arr[l],arr[l+1])
			}
			i=l+1;
			j=ir;
			a=arr[l+1];
			for (;;) {
				i++; while (arr[i] < a) i++;
				j--; while (arr[j] > a) j--;
				if (j < i) break;
				SWAP(arr[i],arr[j]);
			}
			arr[l+1]=arr[j];
			arr[j]=a;
			jstack += 2;

			if (ir-i+1 >= j-l) {
				istack[jstack]=ir;
				istack[jstack-1]=i;
				ir=j-1;
			} else {
				istack[jstack]=j-1;
				istack[jstack-1]=l;
				l=i;
			}
		}
	}
}

void* qsort_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;
    uint32_t i;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[QSORT] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Sorting %u floating points.\n", ARRAY_SIZE);
        for(i = 0; i < ARRAY_SIZE; ++i)
        {
            arr[i] = (float)rand() / (float)(rand() + 1.0);
        }
        sort(ARRAY_SIZE);
        for(i = 1; i < ARRAY_SIZE; ++i)
        {
            if(arr[i] < arr[i - 1])
            {
                OUTPUT("Error, sorting did not go well\n");
            }
        }
        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[QSORT] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_qsort;
    PROCESS_ATTRIBUTE_TYPE th_attr_qsort;

    RETURN_CODE_TYPE       ret_type;


    /* Set QSORT manipulation process */
    th_attr_qsort.ENTRY_POINT   = qsort_thread;
    th_attr_qsort.DEADLINE      = HARD;
    th_attr_qsort.PERIOD        = 100000000;
    th_attr_qsort.STACK_SIZE    = 1000;
    th_attr_qsort.TIME_CAPACITY = 100000000;
    th_attr_qsort.BASE_PRIORITY = 1;
    memcpy(th_attr_qsort.NAME, "QSORT_A653\0", 9 * sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_qsort, &thread_qsort, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create QSORT process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_qsort, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start QSORT process[%d]\n", ret_type);
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
