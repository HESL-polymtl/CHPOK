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

#include "standalone_libs.h"

void* memcpy (void* to, const void*  from, size_t n)
{
  char *cto = (char *)to;
  const char *cfrom = (const char *)from;

  for (; n > 0; n--)
  {
       *cto++ = *cfrom++;
  }

  return (to);
}