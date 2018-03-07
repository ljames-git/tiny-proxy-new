#ifndef __HTTPS_CONNECTION_H__
#define __HTTPS_CONNECTION_H__

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "HttpConnection.h"

#define HTTPS_CONN_CONNECTED 2

class CHttpsConnection: public CHttpConnection
{
public:
    CHttpsConnection();
    CHttpsConnection(sockaddr_in *addr_info, int type, CHttpConnection *manager = NULL, IMultiPlexer *multi_plexer = NULL);
    virtual ~CHttpsConnection();

    virtual int on_connected();

    int handshake();
    int do_ssl_read();
    int do_ssl_write();

public:
    virtual int before_read();
    virtual int before_write();
    virtual int handle_read(IIoHandler **h);
    virtual int handle_write(IIoHandler **h);

private:
    int m_write_in_read;
    int m_read_in_write;
    SSL *m_ssl_handle;
    SSL_CTX *m_ssl_context;
};

#endif //__HTTPS_CONNECTION_H__
