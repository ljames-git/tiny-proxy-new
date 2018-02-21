#ifndef __SELECT_MODEL_H__
#define __SELECT_MODEL_H__

#include <sys/select.h>

#include "IoHandler.h"
#include "MultiPlexer.h"

class CSelectModel: public IMultiPlexer
{
public:
    CSelectModel();
    ~CSelectModel();
    

public:
    // implementations of IMultiPlexer interface
    virtual int clear_fd(int fd);
    virtual int clear_write_fd(int fd);
    virtual int set_timeout(int milli_sec);
    virtual int set_read_fd(int fd, IIoHandler *handler);
    virtual int set_write_fd(int fd, IIoHandler *handler);

    virtual int run();

private:
    // model timeout, millisecond
    int m_timeout;
    fd_set m_read_set;
    fd_set m_write_set;
    fd_set m_exception_set;
    IIoHandler *m_handlers[FD_SETSIZE];
};

#endif //__SELECT_MODEL_H__
