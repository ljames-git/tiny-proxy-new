#ifndef __CONNECTION_MAKERS_H__
#define __CONNECTION_MAKERS_H__

#include "TcpConnection.h"
#include "HttpConnection.h"
#include "ConnectionFactory.h"

class CTcpConnectionFactory: public IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, IMultiPlexer *multi_plexer)
    {
        return CTcpConnection::instance_from_sock(sock, multi_plexer);
    }
};

class CHttpConnectionFactory: public IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, IMultiPlexer *multi_plexer)
    {
        return CHttpConnection::instance_from_sock(sock, multi_plexer);
    }
};

#endif //__CONNECTION_MAKERS_H__
