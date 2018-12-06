/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Felipe Gohring de Magalhaes
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Global statistics tools and instrumentation.
 ******************************************************************************/

#ifndef __STANDALONE_LIBS_H_
#define __STANDALONE_LIBS_H_


#ifndef PI
#define PI 3.14159265359f
#endif
#ifndef PHI
#define PHI      ((1.0+sqrt(5.0))/2.0)         /* the golden number    */
#define INV_PHI  (1.0/PHI)                     /* the golden ratio     */
#endif

#define BITSPERLONG 32

#define TOP2BITS(x) ((x & (3L << (BITSPERLONG-2))) >> (BITSPERLONG-2))

#define deg2rad(d) ((d)*PI/180)
#define rad2deg(r) ((r)*180/PI)

#define MIN(v1, v2) (v1 < v2 ? v1 : v2)

typedef unsigned int size_t;

struct int_sqrt {
      unsigned sqrt,
               frac;
};

extern void* memcpy (void* to, const void*  from, size_t n);

extern void usqrt(unsigned long x, struct int_sqrt *q);
extern double sqrt(double val);
extern double sin(double rad);
extern double fabs(double n);
extern double cos(double x);
extern double pow(double n, double powval);
extern double atan(double x);
extern double acos(double x);

extern void srand(unsigned int seed);
extern unsigned int rand(void);

#endif /* __STANDALONE_LIBS_H_ */