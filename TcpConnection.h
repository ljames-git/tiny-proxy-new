#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <arpa/inet.h>

#include "IoHandler.h"
#include "MultiPlexer.h"

#define TCP_READ_BUF_LEN 4096

class CTcpConnection: public IIoHandler
{
public:
    CTcpConnection();
    CTcpConnection(sockaddr_in *addr_info, IMultiPlexer *multi_plexer = NULL);
    virtual ~CTcpConnection();

    int start(IMultiPlexer *multi_plexer = NULL);

    static CTcpConnection *instance_from_sock(int sock, IMultiPlexer *multi_plexer);

public:
    // implementation of IIoHandler

    // 0: OK, other: remove sock
    virtual int handle_read(IIoHandler **h = NULL);
    
    // 0: OK, other: remove sock
    virtual int handle_write(IIoHandler **h = NULL);

    virtual int handle_error();

protected:
    virtual int on_data(char *buf, int size);

protected:
    int m_sock;
    IMultiPlexer *m_multi_plexer;
    sockaddr_in m_addr_info;
    char m_read_buf[TCP_READ_BUF_LEN];
};

#endif //__TCP_CONNECTION_H__
