#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include "TcpConnection.h"
#include "ConnectionManager.h"

class CHttpHeader
{
public:
    CHttpHeader();
    ~CHttpHeader();

private:
    int m_done;

};

class CHttpConnection: public CTcpConnection, public IConnectionManager
{
public:
    CHttpConnection();
    CHttpConnection(sockaddr_in *addr_info, IMultiPlexer *multi_plexer = NULL);
    virtual ~CHttpConnection();

    virtual int manage(int sock, int status);

    static CHttpConnection *instance_from_sock(int sock, IMultiPlexer *multi_plexer);

protected:
    virtual int on_data(char *buf, int size);
};

#endif //__HTTP_CONNECTION_H__
