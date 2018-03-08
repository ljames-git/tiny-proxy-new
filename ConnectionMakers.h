#ifndef __CONNECTION_MAKERS_H__
#define __CONNECTION_MAKERS_H__

#include "TcpConnection.h"
#include "HttpConnection.h"
#include "HttpsConnection.h"
#include "ConnectionFactory.h"

class CTcpConnectionFactory: public IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, int type, IMultiPlexer *multi_plexer)
    {
        return CTcpConnection::instance_from_sock(sock, type, multi_plexer);
    }
};

class CHttpConnectionFactory: public IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, int type, IMultiPlexer *multi_plexer)
    {
        return CHttpConnection::instance_from_sock(sock, type, multi_plexer);
    }
};

class CHttpsConnectionFactory: public IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, int type, IMultiPlexer *multi_plexer)
    {
        return CHttpsConnection::instance_from_sock(sock, type, multi_plexer);
    }
};

#endif //__CONNECTION_MAKERS_H__
