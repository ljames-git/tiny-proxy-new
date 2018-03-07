#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common.h"
#include "SelectModel.h"


CSelectModel::CSelectModel() :
    m_timeout(0)
{
    FD_ZERO(&m_read_set);
    FD_ZERO(&m_write_set);
    FD_ZERO(&m_exception_set);
    memset(m_handlers, 0, sizeof(m_handlers));
}

CSelectModel::~CSelectModel()
{
}

int CSelectModel::set_read_fd(int fd, IIoHandler *handler)
{
    if (fd < 0 || fd >= FD_SETSIZE || !handler)
    {
        LOG_WARN("failed to set read fd: %d", fd);
        return -1;
    }

    if (m_handlers[fd] && handler != m_handlers[fd])
    {
        LOG_WARN("failed to set read fd, has handler, fd: %d", fd);
        return -2;
    }

    FD_SET(fd, &m_read_set);
    FD_SET(fd, &m_exception_set);
    m_handlers[fd] = handler;
    return 0;
}

int CSelectModel::set_write_fd(int fd, IIoHandler *handler)
{
    if (fd < 0 || fd >= FD_SETSIZE || !handler)
    {
        LOG_WARN("failed to set write fd: %d", fd);
        return -1;
    }


    if (m_handlers[fd] && handler != m_handlers[fd])
    {
        LOG_WARN("failed to set read fd, has handler, fd: %d", fd);
        return -2;
    }

    FD_SET(fd, &m_write_set);
    FD_SET(fd, &m_exception_set);
    m_handlers[fd] = handler;
    return 0;
}

int CSelectModel::clear_write_fd(int fd)
{
    if (fd < 0 || fd >= FD_SETSIZE)
        return -1;

    FD_CLR(fd, &m_write_set);
    return 0;
}

int CSelectModel::clear_fd(int fd)
{
    if (fd < 0 || fd >= FD_SETSIZE)
        return -1;

    FD_CLR(fd, &m_read_set);
    FD_CLR(fd, &m_write_set);
    FD_CLR(fd, &m_exception_set);
    m_handlers[fd] = NULL;
    return 0;
}

int CSelectModel::set_timeout(int milli_sec)
{
    m_timeout = milli_sec;
    return 0;
}

int CSelectModel::run()
{
    struct timeval ctv, tv, *ptv;
    ptv = NULL;
    if (m_timeout > 0)
    {
        tv.tv_sec = m_timeout / 1000;
        tv.tv_usec = (m_timeout % 1000) * 1000;
        ctv = tv;
        ptv = &tv;
    }

    for (;;)
    {
        int ret = 0; 
        fd_set rset, wset, eset;
        for (rset = m_read_set, wset = m_write_set, eset = m_exception_set;
                (ret = select(FD_SETSIZE, &rset, &wset, &eset, ptv)) >= 0;
                rset = m_read_set, wset = m_write_set, eset = m_exception_set, tv = ctv)
        {
            if (ret == 0)
            {
                continue;
            }

            for (int fd = 0; fd < FD_SETSIZE; fd++)
            {
                if (FD_ISSET(fd, &eset))
                {
                    IIoHandler *handler = m_handlers[fd];
                    if (handler == NULL)
                    {
                        // fd might be closed by other threads
                        close(fd);
                        clear_fd(fd);
                    }
                    else
                    {
                        close(fd);
                        clear_fd(fd);
                        handler->handle_error();
                        delete handler;
                    }
                }

                if (FD_ISSET(fd, &rset))
                {
                    IIoHandler *handler = m_handlers[fd];
                    if (handler == NULL)
                    {
                        // fd might be closed by other threads
                        close(fd);
                        clear_fd(fd);
                    }
                    else
                    {
                        ret = handler->before_read();
                        if (ret < 0)
                        {
                            close(fd);
                            clear_fd(fd);
                            delete handler;
                            continue;
                        }
                        else if (ret > 0)
                        {
                            goto CODE_WRITE;
                        }

                        IIoHandler *h = NULL;
                        ret = handler->handle_read(&h);
                        if (ret < 0)
                        {
                            close(fd);
                            clear_fd(fd);
                            delete handler;
                        }
                        else if (ret > 0)
                        {
                            if (h && set_read_fd(ret, h) != 0)
                                delete h;
                        }
                    }
                }

CODE_WRITE:
                if (FD_ISSET(fd, &wset))
                {
                    IIoHandler *handler = m_handlers[fd];
                    if (handler == NULL)
                    {
                        // fd might be closed by other threads
                        close(fd);
                        clear_fd(fd);
                        continue;
                    }

                    ret = handler->before_write();
                    if (ret < 0)
                    {
                        close(fd);
                        clear_fd(fd);
                        delete handler;
                        continue;
                    }
                    else if (ret > 0)
                    {
                        continue;
                    }

                    IIoHandler *h = NULL;
                    ret = handler->handle_write(&h);
                    if (ret < 0)
                    {
                        close(fd);
                        clear_fd(fd);
                        delete handler;
                    }
                }
            }
        }

        LOG_WARN("select error, errno: %d", errno);
    }
    return 0;
}
