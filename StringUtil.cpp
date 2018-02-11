#include <string.h>

#include "StringUtil.h"

char *CStringUtil::trim(char *src, const char *delim)
{
    if (src == NULL)
        return NULL;
    if (!delim || strlen(delim) == 0)
        return src;

    int table[256] = {0};
    for (const char *p = delim; *p; p++)
    {
        if (*p > 0)
            table[(int)*p] = 1;
    }

    char *start = src;
    char *end = NULL;
    for (char *p = src; *p; p++)
    {
        if (!table[(int)*p])
        {
            end = NULL;
            continue;
        }

        if (start == p)
            start++;
        else
            end = p;
    }
    if (end)
        *end = 0;
    return start;
}
