#ifndef __UTILITY_H__
#define __UTILITY_H__

class CUtility
{
public:
    // range: [from, to)
    static int range_rand(int from, int to);
    // range: [0, to)
    static int range_rand(int to);
    // int style IPs, net bytes
    static int host2ips(const char *hostname, unsigned int *ips, int size);
    // rand ip form host2ips, net bytes
    static unsigned int host2randip(const char *hostname);
};

#endif //__UTILITY_H__
