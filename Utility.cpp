#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

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

int CUtility::host2ips(const char *hostname, unsigned int *ips, int size) 
{
    if (!hostname || !ips || size <= 0)
        return -1;

    struct hostent hostret, *hp;
    size_t hstbuflen = 1024;
    char *tmphstbuf;
    int res, herr;

    tmphstbuf = (char *)malloc(hstbuflen);
    while ((res = gethostbyname_r (hostname, &hostret, tmphstbuf, hstbuflen, &hp, &herr)) == ERANGE)
    {
        /* Enlarge the buffer.  */
        hstbuflen *= 2;
        tmphstbuf = (char *)realloc(tmphstbuf, hstbuflen);
    }

    /*  Check for errors.  */
    if (res || hp == NULL)
    {
        free (tmphstbuf);
        return -1;
    }

    int r = 0;
    for (int i = 0; hp->h_addr_list[i] && i < size; i++)
    {
        ips[i] = *((unsigned int *)hp->h_addr_list[i]);
        r++;
    }
    free (tmphstbuf);
    return r;
}

unsigned int CUtility::host2randip(const char *hostname)
{
    unsigned int ips[64];
    int n = host2ips(hostname, ips, sizeof(ips) / sizeof(ips[0]));
    if (n <= 0)
        return -1;
    return ips[range_rand(n)];
}
