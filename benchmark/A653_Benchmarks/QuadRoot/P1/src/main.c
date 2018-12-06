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
#define EQU_COUNT        10000
#define COMPUTATION_LOAD 60

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/


double equ_array[EQU_COUNT][3];

double* a;
double x1[2];
double x2[2];

int flag;

int  qurt();


double fabs(double n)
{
  double f;

  if (n >= 0) f = n;
  else f = -n;
  return f;
}

double sqrt(val)
double val;
{
  double x = val/10;

  double dx;

  double diff;
  double min_tol = 0.00001;

  int i, flag;

  flag = 0;
  if (val == 0 ) x = 0;
  else {
    for (i=1;i<20;i++)
      {
	if (!flag) {
	  dx = (val - (x*x)) / (2.0 * x);
	  x = x + dx;
	  diff = val - (x*x);
	  if (fabs(diff) <= min_tol) flag = 1;
	}
	else
	  x =x;
      }
  }
  return (x);
}

int  qurt()
{
	double  d, w1, w2;

	if(a[0] == 0.0) return(999);
	d = a[1]*a[1] - 4 * a[0] * a[2];
	w1 = 2.0 * a[0];
	w2 = sqrt(fabs(d));
	if(d > 0.0)
	{
		 flag = 1;
		 x1[0] = (-a[1] + w2) / w1;
		 x1[1] = 0.0;
		 x2[0] = (-a[1] - w2) / w1;
		 x2[1] = 0.0;
		 return(0);
	}
	else if(d == 0.0)
	{
		 flag = 0;
		 x1[0] = -a[1] / w1;
		 x1[1] = 0.0;
		 x2[0] = x1[0];
		 x2[1] = 0.0;
		 return(0);
	}
	else
	{
		 flag = -1;
		 w2 /= w1;
		 x1[0] = -a[1] / w1;
		 x1[1] = w2;
		 x2[0] = x1[0];
		 x2[1] = -w2;
		 return(0);
	}
}

void* qurt_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i, j;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[QURT] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Compution quadratic roots (%u)\n", EQU_COUNT);
        for(i = 0; i < EQU_COUNT; ++i)
        {
            a = equ_array[i];
            for(j = 0; j < COMPUTATION_LOAD; ++j)
            {
                qurt();
            }
        }

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[QURT] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_qurt;
    PROCESS_ATTRIBUTE_TYPE th_attr_qurt;

    RETURN_CODE_TYPE       ret_type;


    /* Set QURT manipulation process */
    th_attr_qurt.ENTRY_POINT   = qurt_thread;
    th_attr_qurt.DEADLINE      = HARD;
    th_attr_qurt.PERIOD        = 100000000;
    th_attr_qurt.STACK_SIZE    = 1000;
    th_attr_qurt.TIME_CAPACITY = 100000000;
    th_attr_qurt.BASE_PRIORITY = 1;
    memcpy(th_attr_qurt.NAME, "QURT_A653\0", 9 * sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_qurt, &thread_qurt, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create QURT process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_qurt, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start QURT process[%d]\n", ret_type);
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
