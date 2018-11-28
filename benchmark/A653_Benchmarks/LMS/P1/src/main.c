/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: A LMS adaptive signal enhancement. The input signal is a sine
 *     wave with added white noise.
 *     The detailed description is in the program source code.
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

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define COMPUTATION_LOAD 15

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/
#ifndef RAND_MAX
#define RAND_MAX 32768
#endif
#define PI 3.14159265358979323846


/* function prototypes for fft and filter functions */

static float gaussian(void);

#define N 201
#define L 20            /* filter order, (length L+1) */

/* set convergence parameter */
float mu = 0.01;


int rand()
{
  static unsigned long next = 1;

  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

static float log(r)
float r;
{
  return 4.5;
}

static float fabs(float n)
{
  float f;

  if (n >= 0) f = n;
  else f = -n;
  return f;
}

static float sqrt(val)
float val;
{
  float x = val/10;

  float dx;

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


static float sin(rad)
float rad;
{
  float app;

  float diff;
  int inc = 1;

  while (rad > 2*PI)
	rad -= 2*PI;
  while (rad < -2*PI)
    rad += 2*PI;
  app = diff = rad;
   diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  while(fabs(diff) >= 0.00001) {
    diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  }

  return(app);
}

static float gaussian()
{
    static int ready = 0;       /* flag to indicated stored value */
    static float gstore;        /* place to store other value */
    static float rconst1 = (float)(2.0/RAND_MAX);
    static float rconst2 = (float)(RAND_MAX/2.0);
    float v1,v2,r,fac;
    float gaus;

/* make two numbers if none stored */
    if(ready == 0) {
            v1 = (float)rand() - rconst2;
            v2 = (float)rand() - rconst2;
            v1 *= rconst1;
            v2 *= rconst1;
            r = v1*v1 + v2*v2;
        while (r > 1.0f) {
            v1 = (float)rand() - rconst2;
            v2 = (float)rand() - rconst2;
            v1 *= rconst1;
            v2 *= rconst1;
            r = v1*v1 + v2*v2;
        }       /* make radius less than 1 */

/* remap v1 and v2 to two Gaussian numbers */
        fac = sqrt(-2.0f*log(r)/r);
        gstore = v1*fac;        /* store one */
        gaus = v2*fac;          /* return one */
        ready = 1;              /* set ready flag */
    }

    else {
        ready = 0;      /* reset ready flag for next pair */
        gaus = gstore;  /* return the stored one */
    }

    return(gaus);
}

/*
      function lms(x,d,b,l,mu,alpha)

Implements NLMS Algorithm b(k+1)=b(k)+2*mu*e*x(k)/((l+1)*sig)

x      = input data
d      = desired signal
b[0:l] = Adaptive coefficients of lth order fir filter
l      = order of filter (> 1)
mu     = Convergence parameter (0.0 to 1.0)
alpha  = Forgetting factor   sig(k)=alpha*(x(k)**2)+(1-alpha)*sig(k-1)
         (>= 0.0 and < 1.0)

returns the filter output
*/

float lms(float x,float d,float *b,int l,
                  float mu,float alpha)
{
    int ll;
    float e,mu_e,y;
    static float px[51];      /* max L = 50 */
    static float sigma = 2.0; /* start at 2 and update internally */

    px[0]=x;

/* calculate filter output */
    y=b[0]*px[0];
#ifdef DEBUG
    printf("l=%d\n",l);
#endif
    for(ll = 1 ; ll <= l ; ll++)
        y=y+b[ll]*px[ll];

/* error signal */
    e=d-y;

/* update sigma */
    sigma=alpha*(px[0]*px[0])+(1-alpha)*sigma;
    mu_e=mu*e/sigma;

/* update coefficients */
    for(ll = 0 ; ll <= l ; ll++)
        b[ll]=b[ll]+mu_e*px[ll];
/* update history */
    for(ll = l ; ll >= 1 ; ll--)
        px[ll]=px[ll-1];

    return(y);
}

void* lms_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("[LMS] Cannot get partition status [%d]\n", ret_type);
        return (void*)1;
    }

    while(1)
    {
        printf("Applying LMS\n");

        float d[N],b[21];
        float signal_amp,noise_amp,arg,x;
        int k;
        int i;

        for(i = 0; i < COMPUTATION_LOAD; ++i)
        {

        /* create signal plus noise */
            signal_amp = sqrt(2.0);
            noise_amp = 0.2*sqrt(12.0);
            arg = 2.0*PI/20.0;
            for(k = 0 ; k < N ; k++) {
                d[k] = signal_amp*sin(arg*k) + noise_amp*gaussian();
            }

        /* scale based on L */
            mu = 2.0*mu/(L+1);

            x = 0.0;
            for(k = 0 ; k < N ; k++) {
                lms(x,d[k],b,L,mu,0.01);
        /* delay x one sample */
                x = d[k];
            }
        }
        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[LMS] Cannot achieve periodic wait [%d]\n", ret_type);
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
    PROCESS_ID_TYPE        thread_lms;
    PROCESS_ATTRIBUTE_TYPE th_attr_lms;

    RETURN_CODE_TYPE       ret_type;


    /* Set LMS manipulation process */
    th_attr_lms.ENTRY_POINT   = lms_thread;
    th_attr_lms.DEADLINE      = HARD;
    th_attr_lms.PERIOD        = 100000000;
    th_attr_lms.STACK_SIZE    = 4096;
    th_attr_lms.TIME_CAPACITY = 100000000;
    th_attr_lms.BASE_PRIORITY = 1;
    memcpy(th_attr_lms.NAME, "LMS_A653\0", 9 * sizeof(char));

    printf("Init P0 partition\n");

    /* Create processes */
    CREATE_PROCESS(&th_attr_lms, &thread_lms, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create LMS process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_lms, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start LMS process[%d]\n", ret_type);
        return -1;
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("P0 partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch P0 partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}
