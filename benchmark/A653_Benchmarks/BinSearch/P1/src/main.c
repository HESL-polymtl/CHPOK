/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Root computation of quadratic equations.
 *     The real and imaginary parts of the solution are stored in the
 *     array x1[] and x2[].
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
#define COMPUTATION_LOAD 10000

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/


struct DATA {
  int  key;
  int  value;
}  ;

#ifdef DEBUG
	int cnt1;
#endif

struct DATA data[15] = { {1, 100},
	     {5,200},
	     {6, 300},
	     {7, 700},
	     {8, 900},
	     {9, 250},
	     {10, 400},
	     {11, 600},
	     {12, 800},
	     {13, 1500},
	     {14, 1200},
	     {15, 110},
	     {16, 140},
	     {17, 133},
	     {18, 10} };

int binary_search(int x)
{
  int fvalue, mid, up, low ;

  low = 0;
  up = 14;
  fvalue = -1 /* all data are positive */ ;
  while (low <= up) {
    mid = (low + up) >> 1;
    if ( data[mid].key == x ) {  /*  found  */
      up = low - 1;
      fvalue = data[mid].value;
#ifdef DEBUG
	OUTPUT("FOUND!!\n");
#endif
    }
    else  /* not found */
      if ( data[mid].key > x ) 	{
	up = mid - 1;
#ifdef DEBUG
	OUTPUT("MID-1\n");
#endif
      }
      else   {
             	low = mid + 1;
#ifdef DEBUG
	OUTPUT("MID+1\n");
#endif
      }
#ifdef DEBUG
	cnt1++;
#endif
  }
#ifdef DEBUG
	OUTPUT("Loop Count : %d\n", cnt1);
#endif
  return fvalue;
}

void* binsrch_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[BINSRCH] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Searching for %u elements\n", COMPUTATION_LOAD);
        for(i = 0; i < COMPUTATION_LOAD; ++i)
        {
            binary_search(rand() % 15);
        }

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[BINSRCH] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_binsrch;
    PROCESS_ATTRIBUTE_TYPE th_attr_binsrch;

    RETURN_CODE_TYPE       ret_type;


    /* Set BINSRCH manipulation process */
    th_attr_binsrch.ENTRY_POINT   = binsrch_thread;
    th_attr_binsrch.DEADLINE      = HARD;
    th_attr_binsrch.PERIOD        = 100000000;
    th_attr_binsrch.STACK_SIZE    = 1000;
    th_attr_binsrch.TIME_CAPACITY = 100000000;
    th_attr_binsrch.BASE_PRIORITY = 1;
    memcpy(th_attr_binsrch.NAME, "BINSRCH_A653\0", 9 * sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_binsrch, &thread_binsrch, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create BINSRCH process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_binsrch, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start BINSRCH process[%d]\n", ret_type);
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
