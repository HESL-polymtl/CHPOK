/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
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

/* I5 750 Intel processor */
#define CACHE_SIZE_L1_INST 32768      /* Bytes */
#define CACHE_SIZE_L1_DATA 32768      /* Bytes */
#define CACHE_SIZE_L2      262144     /* Bytes */
#define CACHE_SIZE_L3      8388608    /* Bytes */

#define CACHE_LINE_SIZE_L1_INST 64    /* Bytes */
#define CACHE_LINE_SIZE_L1_DATA 64    /* Bytes */
#define CACHE_LINE_SIZE_L2      64    /* Bytes */
#define CACHE_LINE_SIZE_L3      64    /* Bytes */

#define CACHE_ASSOC_NUMBER_L1_INST 4  /* Ways */
#define CACHE_ASSOC_NUMBER_L1_DATA 8  /* Ways */
#define CACHE_ASSOC_NUMBER_L2      8  /* Ways */
#define CACHE_ASSOC_NUMBER_L3      16 /* Ways */

#define TLB_SIZE_INST   128   /* 4K Entries */
#define TLB_SIZE_DATA   64    /* 4K Entries */
#define TLB_SIZE_SHARED 512   /* 4K Entries */

#define TLB_ASSOC_NUMBER_INST   4 /* Ways */
#define TLB_ASSOC_NUMBER_DATA   4 /* Ways */
#define TLB_ASSOC_NUMBER_SHARED 4 /* Ways */

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

#define ARCH_X86 1
#define ARCH_PPC 0
#define ARCH_ARM 0


#endif /* __CONFIG_H_ */
