#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <arpa/inet.h>

#include "IoHandler.h"
#include "MultiPlexer.h"

#define TCP_READ_BUF_LEN 4096

#define TCP_CONN_TYPE_NONE 0
#define TCP_CONN_TYPE_SERVER 1
#define TCP_CONN_TYPE_CLIENT 2

class CTcpConnection: public IIoHandler
{
public:
    CTcpConnection();
    CTcpConnection(sockaddr_in *addr_info, int type, IMultiPlexer *multi_plexer = NULL);
    virtual ~CTcpConnection();

    int start(IMultiPlexer *multi_plexer = NULL);

    static CTcpConnection *instance_from_sock(int sock, int type, IMultiPlexer *multi_plexer);

public:
    // implementation of IIoHandler

    // 0: OK, other: remove sock
    virtual int handle_read(IIoHandler **h = NULL);
    
    // 0: OK, other: remove sock
    virtual int handle_write(IIoHandler **h = NULL);

    virtual int handle_error();

protected:
    virtual int on_server_data(char *buf, int size);
    virtual int on_client_data(char *buf, int size);

protected:
    int m_sock;
    int m_type;
    IMultiPlexer *m_multi_plexer;
    sockaddr_in m_addr_info;
    char m_read_buf[TCP_READ_BUF_LEN];
};

#endif //__TCP_CONNECTION_H__
