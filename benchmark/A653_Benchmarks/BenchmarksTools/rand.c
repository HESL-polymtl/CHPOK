#include "standalone_libs.h"

static unsigned long int next = 1;

unsigned int rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next) % RAND_MAX;
}

void srand(unsigned int seed)
{
    next = seed;
}