/*******************************************************************************
 * ARINC-653 RTOS Benchmark
 *
 * Developped/Ported by:
 *    Felipe Gohring de Magalhaes
 *    Jean-Baptiste Lefoul
 *    Alexy Torres Aurora Dugo
 *
 *
 * Test: Solve basic mathematical functions, the benchmark can be set to perform
 * large, small or both computation loads.
 *
 * Ported from:
 *     MiBench Version 1.0 (http://vhosts.eecs.umich.edu/mibench/)
 *
 ******************************************************************************/

#include "../../../BenchmarksTools/benc_config.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

/* Note: At least one of them should be uncommented */
#define BASIC_MATH_LARGE 0
#define BASIC_MATH_SMALL 1

/*******************************************************************************
 * PARTITION SPECIFIC VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

extern void SolveCubic(double  a, double  b, double  c, double  d,
                       int  *solutions, double *x);

void large_process(void)
{
      double  a1 = 1.0, b1 = -10.5, c1 = 32.0, d1 = -30.0;
  double  x[3];
  double X;
  int     solutions;
  int i;
  unsigned long l = 0x3fed0169L;
  struct int_sqrt q;

  /* solve soem cubic functions */
  OUTPUT("********* CUBIC FUNCTIONS ***********\n");
  /* should get 3 solutions: 2, 6 & 2.5   */
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = 1.0; b1 = -4.5; c1 = 17.0; d1 = -30.0;
  /* should get 1 solution: 2.5           */
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = 1.0; b1 = -3.5; c1 = 22.0; d1 = -31.0;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = 1.0; b1 = -13.7; c1 = 1.0; d1 = -35.0;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = 3.0; b1 = 12.34; c1 = 5.0; d1 = 12.0;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = -8.0; b1 = -67.89; c1 = 6.0; d1 = -23.6;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = 45.0; b1 = 8.67; c1 = 7.5; d1 = 34.0;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  a1 = -12.0; b1 = -1.7; c1 = 5.3; d1 = 16.0;
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");

  /* Now solve some random equations */
  for(a1=1;a1<10;a1+=1) {
    for(b1=10;b1>0;b1-=.25) {
      for(c1=5;c1<15;c1+=0.61) {
	   for(d1=-1;d1>-5;d1-=.451) {
		SolveCubic(a1, b1, c1, d1, &solutions, x);
		OUTPUT("Solutions:");
		for(i=0;i<solutions;i++)
		  OUTPUT(" %f",x[i]);
		OUTPUT("\n");
	   }
      }
    }
  }


  OUTPUT("********* INTEGER SQR ROOTS ***********\n");
  /* perform some integer square roots */
  for (i = 0; i < 100000; i+=2)
    {
      usqrt(i, &q);
			// remainder differs on some machines
     // OUTPUT("sqrt(%3d) = %2d, remainder = %2d\n",
     OUTPUT("sqrt(%3d) = %2d\n",
	     i, q.sqrt);
    }
  OUTPUT("\n");
  for (l = 0x3fed0169L; l < 0x3fed4169L; l++)
    {
	 usqrt(l, &q);
	 //OUTPUT("\nsqrt(%lX) = %X, remainder = %X\n", l, q.sqrt, q.frac);
	 OUTPUT("sqrt(%lX) = %X\n", l, q.sqrt);
    }


  OUTPUT("********* ANGLE CONVERSION ***********\n");
  /* convert some rads to degrees */
/*   for (X = 0.0; X <= 360.0; X += 1.0) */
  for (X = 0.0; X <= 360.0; X += .001)
    OUTPUT("%3.0f degrees = %.12f radians\n", X, deg2rad(X));
  puts("");
/*   for (X = 0.0; X <= (2 * PI + 1e-6); X += (PI / 180)) */
  for (X = 0.0; X <= (2 * PI + 1e-6); X += (PI / 5760))
    OUTPUT("%.12f radians = %3.0f degrees\n", X, rad2deg(X));


}
void small_process(void)
{
  double  a1 = 1.0, b1 = -10.5, c1 = 32.0, d1 = -30.0;
  double  a2 = 1.0, b2 = -4.5, c2 = 17.0, d2 = -30.0;
  double  a3 = 1.0, b3 = -3.5, c3 = 22.0, d3 = -31.0;
  double  a4 = 1.0, b4 = -13.7, c4 = 1.0, d4 = -35.0;
  double  x[3];
  double X;
  int     solutions;
  int i;
  unsigned long l = 0x3fed0169L;
  struct int_sqrt q;

  /* solve soem cubic functions */
  OUTPUT("********* CUBIC FUNCTIONS ***********\n");
  /* should get 3 solutions: 2, 6 & 2.5   */
  SolveCubic(a1, b1, c1, d1, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");
  /* should get 1 solution: 2.5           */
  SolveCubic(a2, b2, c2, d2, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");
  SolveCubic(a3, b3, c3, d3, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");
  SolveCubic(a4, b4, c4, d4, &solutions, x);
  OUTPUT("Solutions:");
  for(i=0;i<solutions;i++)
    OUTPUT(" %f",x[i]);
  OUTPUT("\n");
  /* Now solve some random equations */
  for(a1=1;a1<10;a1++) {
    for(b1=10;b1>0;b1--) {
      for(c1=5;c1<15;c1+=0.5) {
	for(d1=-1;d1>-11;d1--) {
	  SolveCubic(a1, b1, c1, d1, &solutions, x);
	  OUTPUT("Solutions:");
	  for(i=0;i<solutions;i++)
	    OUTPUT(" %f",x[i]);
	  OUTPUT("\n");
	}
      }
    }
  }

  OUTPUT("********* INTEGER SQR ROOTS ***********\n");
  /* perform some integer square roots */
  for (i = 0; i < 1001; ++i)
    {
      usqrt(i, &q);
			// remainder differs on some machines
     // OUTPUT("sqrt(%3d) = %2d, remainder = %2d\n",
     OUTPUT("sqrt(%3d) = %2d\n",
	     i, q.sqrt);
    }
  usqrt(l, &q);
  //OUTPUT("\nsqrt(%lX) = %X, remainder = %X\n", l, q.sqrt, q.frac);
  OUTPUT("\nsqrt(%lX) = %X\n", l, q.sqrt);


  OUTPUT("********* ANGLE CONVERSION ***********\n");
  /* convert some rads to degrees */
  for (X = 0.0; X <= 360.0; X += 1.0)
    OUTPUT("%3.0f degrees = %.12f radians\n", X, deg2rad(X));
  puts("");
  for (X = 0.0; X <= (2 * PI + 1e-6); X += (PI / 180))
    OUTPUT("%.12f radians = %3.0f degrees\n", X, rad2deg(X));

}

void* large_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;


    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[BASICMATH_LARGE] Cannot get partition status [%d]\n",
               ret_type);
        return (void*)1;
    }

    while(1)
    {
        large_process();

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[BASICMATH_LARGE] Cannot achieve periodic wait [%d]\n",
                   ret_type);
            return (void*)1;
        }
    }

    return (void*)0;
}

void* small_thread(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;


    GET_PARTITION_STATUS(&pr_stat, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("[BASICMATH_SMALL] Cannot get partition status [%d]\n",
               ret_type);
        return (void*)1;
    }

    while(1)
    {
        small_process();

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            OUTPUT("[BASICMATH_SMALL] Cannot achieve periodic wait [%d]\n",
                   ret_type);
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
    RETURN_CODE_TYPE       ret_type;


    OUTPUT("Init MiBench BasicMath partition\n");

#if BASIC_MATH_LARGE
    PROCESS_ID_TYPE        thread_large;
    PROCESS_ATTRIBUTE_TYPE th_attr_large;

    /* Set Large process */
    th_attr_large.ENTRY_POINT   = large_thread;
    th_attr_large.DEADLINE      = HARD;
    th_attr_large.PERIOD        = 1000000000;
    th_attr_large.STACK_SIZE    = 1000;
    th_attr_large.TIME_CAPACITY = 1000000000;
    th_attr_large.BASE_PRIORITY = 1;
    memcpy(th_attr_large.NAME, "BASICMATH_LARGE_A653\0", 21 * sizeof(char));


    CREATE_PROCESS(&th_attr_large, &thread_large, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create LARGE process [%d]\n", ret_type);
        return -1;
    }

    START(thread_large, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start LARGE process[%d]\n", ret_type);
        return -1;
    }

#endif

#if BASIC_MATH_SMALL
    PROCESS_ID_TYPE        thread_small;
    PROCESS_ATTRIBUTE_TYPE th_attr_small;

     /* Set Small process */
    th_attr_small.ENTRY_POINT   = small_thread;
    th_attr_small.DEADLINE      = HARD;
    th_attr_small.PERIOD        = 1000000000;
    th_attr_small.STACK_SIZE    = 1000;
    th_attr_small.TIME_CAPACITY = 1000000000;
    th_attr_small.BASE_PRIORITY = 1;
    memcpy(th_attr_small.NAME, "BASICMATH_SMALL_A653\0", 21 * sizeof(char));


    CREATE_PROCESS(&th_attr_small, &thread_small, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot create SMALL process [%d]\n", ret_type);
        return -1;
    }

    START(thread_small, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot start SMALL process[%d]\n", ret_type);
        return -1;
    }

#endif

    /* Partition has been initialized, now switch to normal mode */
    OUTPUT("MiBench BasicMath partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        OUTPUT("Cannot switch MiBench BasicMath to NORMAL state[%d]\n",
               ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}
