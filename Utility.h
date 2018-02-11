#ifndef __UTILITY_H__
#define __UTILITY_H__

class CUtility
{
public:
    // range: [from, to)
    static int range_rand(int from, int to);
    // range: [0, to)
    static int range_rand(int to);
};

#endif //__UTILITY_H__
