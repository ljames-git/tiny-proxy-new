#include <time.h>
#include <stdlib.h>

#include "Utility.h"

int CUtility::range_rand(int from, int to)
{
    if (from < 0 || to < 0 || from >= to)
        return -1;

    int range = to - from;
    unsigned int seed = time(NULL);
    return from + (int)(rand_r(&seed) * 1.0 / RAND_MAX * range);
}

int CUtility::range_rand(int to)
{
    return range_rand(0, to);
}
