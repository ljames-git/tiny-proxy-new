#ifndef __MULTI_PLEXER_H__
#define __MULTI_PLEXER_H__

#include "Runnable.h"
#include "IoHandler.h"

class IMultiPlexer: public IRunnable
{
public:
    virtual int clear_fd(int fd) = 0;
    virtual int clear_write_fd(int fd) = 0;
    virtual int set_timeout(int milli_sec) = 0;
    virtual int set_read_fd(int fd, IIoHandler *handler) = 0;
    virtual int set_write_fd(int fd, IIoHandler *handler) = 0;
};

#endif //__MULTI_PLEXER_H__
