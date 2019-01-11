/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Transitive closure from:
 *     https://www.ics.uci.edu/~amrm/hdu/DIS_Stressmark/DIS_stressmark.html
 *
 *
 ******************************************************************************/

#include "../../../../A653_Benchmarks/BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define MIN_VERTICES 8
#define MAX_VERTICES 16384
#define MIN_EDGES 0
#define MAX_EDGES 268435456
#define MIN_SEED -2147483647
#define MAX_SEED -1
#define NO_PATH 2147483647

#define MIN_EDGS 0
#define MAX_EDGE 255

#define assert(expr) ( \
    (expr) ? \
    ((void) 0) : \
    OUTPUT("ASSERT Failed") \
)

/*
 * main()
 */

#define N         50
#define M         65

unsigned int din_arr[N * N];
unsigned int dout_arr[N * N];

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

int randInt(int minimum_number, int max_number) {
    return rand() % (max_number + 1 - minimum_number) + minimum_number;
}

void closure()
{
    unsigned int *din, *dout;
    unsigned int n;
    unsigned int m;
    unsigned int i, j, k;

    unsigned int sum;
    n = N;
    m = M;


    assert((n >= MIN_VERTICES) && (n <= MAX_VERTICES));
    assert((m >= MIN_EDGES) && (m <= MAX_EDGES));
    assert (m <= n*n);

    din = din_arr;
    dout = dout_arr;

    for (i=0; i<n*n; i++){
        *(din + i) = NO_PATH;
        *(dout + i) = NO_PATH;
    }

    for (k=0; k<m; k++){
        i = randInt(0, n);
        j = randInt(0, n);
        *(din + j*n + i) = randInt(MIN_EDGES, MAX_EDGES);
    }

    for (k=0; k<n; k++){
        unsigned int old;
        unsigned int new1;
        unsigned int *dtemp;

        for (i=0; i<n; i++){
            for (j=0; j<n; j++){
                old = *(din + j*n + i);
                new1 = *(din + j*n + k) + *(din + k*n + i);
                *(dout + j*n + i) = (new1 < old ? new1: old);
                assert (*(dout + j*n + i) <= NO_PATH);
                assert (*(dout + j*n + i) <= *(din + j*n + i));
            }
        }
        dtemp = dout;
        dout = din;
        din = dtemp;
    }

    for (j=0; j<n; j++){
        sum = 0;
        for (i=0; i<n; i++){
            if (*(din + j*n + i) != NO_PATH)
                sum += *(din + j*n + i);
        }
    }
    for (i=0; i<n; i++){
        sum = 0;
        for (j=0; j<n; j++){
            if (*(din + j*n + i) != NO_PATH)
                sum += *(din+j*n+i);
        }
    }
}

void* transitive_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;


    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[TRANSITIVE] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        OUTPUT("Transitive closure\n");
        closure();

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[TRANSITIVE] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_transitive;
    PROCESS_ATTRIBUTE_TYPE th_attr_transitive;

    RETURN_CODE_TYPE       ret_type;


    /* Set TRANSITIVE manipulation process */
    th_attr_transitive.ENTRY_POINT   = transitive_thread;
    th_attr_transitive.DEADLINE      = HARD;
    th_attr_transitive.PERIOD        = 1000000000;
    th_attr_transitive.STACK_SIZE    = 4096;
    th_attr_transitive.TIME_CAPACITY = 1000000000;
    th_attr_transitive.BASE_PRIORITY = 1;
    memcpy(th_attr_transitive.NAME, "TRANSITIVE_A653\0", 13* sizeof(char));

    OUTPUT("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_transitive, &thread_transitive, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create TRANSITIVE process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_transitive, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start TRANSITIVE process[%d]\n", ret_type);
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
