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

public:
    virtual int before_read();
    virtual int before_write();

private:
    SSL *m_ssl_handle;
    SSL_CTX *m_ssl_context;
};

#endif //__HTTPS_CONNECTION_H__
