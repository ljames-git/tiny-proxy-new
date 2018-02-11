#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

class CStringUtil
{
public:
    static char *trim(char *src, const char *delim = " \t\r\n");
};

#endif //__STRING_UTIL_H__
