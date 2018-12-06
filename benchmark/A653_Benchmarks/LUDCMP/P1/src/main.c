/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Simultaneous linear equations by LU decomposition.
 *     The arrays a[][] and b[] are input and the array x[] is output
 *     row vector.
 *     The variable n is the number of equations.
 *
 * Ported from:
 *     SNU-RT Benchmarks (http://www.cprover.org/goto-cc/examples/snu.html)
 *
 ******************************************************************************/


#include "../../../BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/
#define EQU_COUNT 100

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/


double a[EQU_COUNT][EQU_COUNT], b[EQU_COUNT], x[EQU_COUNT];

int ludcmp(int n, double eps)
{

	int i, j, k;
	double w, y[100];

	for(i = 0; i < n; i++)
	{
			if(fabs(a[i][i]) <= eps) return(1);
			for(j = i+1; j <= n; j++)
			{
			  w = a[j][i];
			  if(i != 0)
			    for(k = 0; k < i; k++)
			      w -= a[j][k] * a[k][i];
			  a[j][i] = w / a[i][i];
			}
			for(j = i+1; j <= n; j++)
			  {
			    w = a[i+1][j];
			    for(k = 0; k <= i; k++)
			      w -= a[i+1][k] * a[k][j];
			    a[i+1][j] = w;
			  }
	}
	y[0] = b[0];
	for(i = 1; i <= n; i++)
	  {
	    w = b[i];
	    for(j = 0; j < i; j++)
	      w -= a[i][j] * y[j];
	    y[i] = w;
	  }
	x[n] = y[n] / a[n][n];
	for(i = n-1; i >= 0; i--)
	  {
	    w = y[i];
	    for(j = i+1; j <= n; j++)
	      w -= a[i][j] * x[j];
	    x[i] = w / a[i][i] ;
	  }
	return(0);

}


void* ludcmp_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[LUDCMP] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Solving %u equations\n", EQU_COUNT);

        int i, j, n = EQU_COUNT, chkerr;
        double eps, w;

        eps = 1.0e-6;

        for(i = 0; i <= n; i++)
        {
                w = 0.0;
                for(j = 0; j <= n; j++)
                {
                        a[i][j] = (i + 1) + (j + 1);
                        if(i == j) a[i][j] *= 10.0;
                        w += a[i][j];
                }
                b[i] = w;
        }

        chkerr = ludcmp(n, eps);
        OUTPUT("Result: %d\n", chkerr);


        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[LUDCMP] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_ludcmp;
    PROCESS_ATTRIBUTE_TYPE th_attr_ludcmp;

    RETURN_CODE_TYPE       ret_type;


    /* Set LUDCMP manipulation process */
    th_attr_ludcmp.ENTRY_POINT   = ludcmp_thread;
    th_attr_ludcmp.DEADLINE      = HARD;
    th_attr_ludcmp.PERIOD        = 100000000;
    th_attr_ludcmp.STACK_SIZE    = 16384;
    th_attr_ludcmp.TIME_CAPACITY = 100000000;
    th_attr_ludcmp.BASE_PRIORITY = 1;
    memcpy(th_attr_ludcmp.NAME, "LUDCMP_A653\0", 9 * sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_ludcmp, &thread_ludcmp, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create LUDCMP process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_ludcmp, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start LUDCMP process[%d]\n", ret_type);
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
