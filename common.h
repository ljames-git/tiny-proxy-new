#ifndef __COMMON_H__
#define __COMMON_H__

#include <time.h>
#include <stdio.h>

#ifdef OPEN_LOG_ALL
#define OPEN_LOG_DEBUG
#endif //OPEN_LOG_ALL

#ifdef OPEN_LOG_DEBUG
#define OPEN_LOG_STAT
#endif //OPEN_LOG_DEBUG

#ifdef OPEN_LOG_STAT
#define OPEN_LOG_INFO
#endif //OPEN_LOG_STAT

#define LOG_LEVEL_DEBUG "DEBUG"
#define LOG_LEVEL_STAT "STAT"
#define LOG_LEVEL_INFO "INFO"
#define LOG_LEVEL_WARN "WARN"
#define LOG_LEVEL_ERROR "ERROR"

#define LOG_TAG(level, fmt, arg...) \
{\
    time_t t = time(NULL);\
    struct tm *local = localtime(&t); \
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d [%s] [%s:%s] " \
            fmt \
            "\n",\
            local->tm_year + 1900,\
            local->tm_mon + 1,\
            local->tm_mday + 1,\
            local->tm_hour,\
            local->tm_min,\
            local->tm_sec,\
            level,\
            __FILE__,\
            __FUNCTION__, ##arg);\
}

#ifdef OPEN_LOG_DEBUG
#define LOG_DEBUG(fmt, arg...) \
{\
    LOG_TAG(LOG_LEVEL_DEBUG, fmt, ##arg);\
}
#else //OPEN_LOG_DEBUG
#define LOG_DEBUG(fmt, arg...) 
#endif //OPEN_LOG_DEBUG

#ifdef OPEN_LOG_INFO
#define LOG_INFO(fmt, arg...) \
{\
    LOG_TAG(LOG_LEVEL_INFO, fmt, ##arg);\
}
#else //OPEN_LOG_INFO
#define LOG_INFO(fmt, arg...) 
#endif //OPEN_LOG_INFO

#ifdef OPEN_LOG_STAT
#define LOG_STAT(fmt, arg...) \
{\
    LOG_TAG(LOG_LEVEL_STAT, fmt, ##arg);\
}
#else //OPEN_LOG_STAT
#define LOG_STAT(fmt, arg...) 
#endif //OPEN_LOG_STAT

#define LOG_WARN(fmt, arg...) \
{\
    LOG_TAG(LOG_LEVEL_WARN, fmt, ##arg);\
}
#define LOG_ERROR(fmt, arg...) \
{\
    LOG_TAG(LOG_LEVEL_ERROR, fmt, ##arg);\
}


#define ERROR_ON_NEG(func) \
{\
    int r = (func);\
    if (r < 0)\
    {\
        LOG_ERROR("error on: "#func);\
        return -9999;\
    }\
}

#define ASSIGN_AND_ERROR_ON_NEG(ret, func) \
{\
    int r = (func);\
    if (r < 0)\
    {\
        LOG_ERROR("error on: "#func);\
        return -9999;\
    }\
    ret = r;\
}

#define ERROR_ON_NON_ZERO(func) \
{\
    int r = (func);\
    if (r != 0)\
    {\
        LOG_ERROR("error on: "#func);\
        return -9999;\
    }\
}

#endif //__COMMON_H__
