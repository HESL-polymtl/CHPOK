/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Felipe Gohring de Magalhaes
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Global tests configuration and settings. You may define architecture
 * specific values here
 ******************************************************************************/

#ifndef __CONFIG_H_
#define __CONFIG_H_

/*******************************************************************************
 * PROCESSOR
 ******************************************************************************/

/* PPC e200z7 */
#define CACHE_SIZE_L1_INST 16384      /* Bytes */
#define CACHE_SIZE_L1_DATA 16384      /* Bytes */
#define CACHE_SIZE_L2      0          /* Bytes */
#define CACHE_SIZE_L3      0          /* Bytes */

#define CACHE_LINE_SIZE_L1_INST 32    /* Bytes */
#define CACHE_LINE_SIZE_L1_DATA 32    /* Bytes */
#define CACHE_LINE_SIZE_L2      0     /* Bytes */
#define CACHE_LINE_SIZE_L3      0     /* Bytes */

#define CACHE_ASSOC_NUMBER_L1_INST 4  /* Ways */
#define CACHE_ASSOC_NUMBER_L1_DATA 4  /* Ways */
#define CACHE_ASSOC_NUMBER_L2      0  /* Ways */
#define CACHE_ASSOC_NUMBER_L3      0  /* Ways */

#define SYSTEM_PAGE_SIZE 4096 /* Bytes */

/* General */
#define CACHE_SETS_NUMBER_L1_INST (CACHE_SIZE_L1_INST /     \
    (CACHE_ASSOC_NUMBER_L1_INST * CACHE_LINE_SIZE_L1_INST))

#define CACHE_SETS_NUMBER_L1_DATA (CACHE_SIZE_L1_DATA /     \
    (CACHE_ASSOC_NUMBER_L1_DATA * CACHE_LINE_SIZE_L1_DATA))

#define CACHE_SETS_NUMBER_L2 (CACHE_SIZE_L2 /     \
    (CACHE_ASSOC_NUMBER_L2 * CACHE_LINE_SIZE_L2))

#define CACHE_SETS_NUMBER_L3 (CACHE_SIZE_L3 /     \
    (CACHE_ASSOC_NUMBER_L3 * CACHE_LINE_SIZE_L3))

/*******************************************************************************
 * ARCHITECTURES
 ******************************************************************************/

#define ARCH_X86 0
#define ARCH_PPC 1
#define ARCH_ARM 0


/*******************************************************************************
 * TOOLS
 ******************************************************************************/

#define OUTPUT_ENABLED 1// Comment this if you don't want the programs to ouput

#include "standalone_libs.h"

/*******************************************************************************
 * OPERATING SYSTEM
 *
 * You can add you own deefine here, but should only uncomment one version of
 * the OPERATING_SYSTEM definition.
 ******************************************************************************/
#define VXWORKX 0
#define CHPOK   1
#define CUSTOM  2

//#define OPERATING_SYSTEM VXWORKX
#define OPERATING_SYSTEM CHPOK
//#define OPERATING_SYSTEM CUSTOM


#if OPERATING_SYSTEM == VXWORK

#if OUTPUT_ENABLED
#define OUTPUT(FMT, ...) printf(FMT, ##__VA_ARGS__)
#else
#define OUTPUT(FMT, ...)
#endif

#elif OPERATING_SYSTEM == CHPOK

#if OUTPUT_ENABLED
#define OUTPUT(FMT, ...) printf(FMT, ##__VA_ARGS__)
#else
#define OUTPUT(FMT, ...)
#endif

#include <stdio.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>

#elif OPERATING_SYSTEM == CUSTOM

#if OUTPUT_ENABLED
/* This counts the number of args */
#define NARGS_SEQ(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define NARGS(...) NARGS_SEQ(##__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)
#define PRIMITIVE_CAT(x, y) x ## y
#define CAT(x, y) PRIMITIVE_CAT(x, y)

#define CUST_PRINT(FMT, ...) CAT(APPLY_, NARGS(##__VA_ARGS__))(FMT, ##__VA_ARGS__)

#define APPLY_0(FMT) \
    Printf(FMT, 0, 0, 0, 0, 0, 0, 0, 0)
#define APPLY_1(FMT, x1) \
    Printf(FMT, (void*)x1, 0, 0, 0, 0, 0, 0, 0)
#define APPLY_2(FMT, x1, x2) \
    Printf(FMT, (void*)x1, (void*)x2, 0, 0, 0, 0, 0, 0)
#define APPLY_3(FMT, x1, x2, x3) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, 0, 0, 0, 0, 0)
#define APPLY_4(FMT, x1, x2, x3, x4) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, (void*)x4, 0, 0, 0, 0)
#define APPLY_5(FMT, x1, x2, x3, x4, x5) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, (void*)x4, (void*)x5, 0, 0, 0)
#define APPLY_6(FMT, x1, x2, x3, x4, x5, x6) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, (void*)x4, (void*)x5, (void*)x6, 0, 0)
#define APPLY_7(FMT, x1, x2, x3, x4, x5, x6, x7) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, (void*)x4, (void*)x5, (void*)x6, (void*)x7, 0)
#define APPLY_8(FMT, x1, x2, x3, x4, x5, x6, x7, x8) \
    Printf(FMT, (void*)x1, (void*)x2, (void*)x3, (void*)x4, (void*)x5, (void*)x6, (void*)x7, (void*)x8)

#define OUTPUT(FMT, ...) CUST_PRINT(FMT, ##__VA_ARGS__)
#else
#define OUTPUT(FMT, ...)
#endif

#endif
/*******************************************************************************
 * INFORMATIONS
 ******************************************************************************/
/*
 *
 * Cache Index : I = (V mod (S / N)) / L
 *     I: Cache line index;
 *     V: Virtual address;
 *     S: Cache size;
 *     N: Number of cache sets;
 *     L: Line size;
 *
 * V + n*(S/N) leads to the same cache index
*/

#endif /* __CONFIG_H_ */
