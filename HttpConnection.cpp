#include "common.h"
#include "HttpConnection.h"

CHttpConnection::CHttpConnection()
{
}

CHttpConnection::CHttpConnection(sockaddr_in *addr_info, IMultiPlexer *multi_plexer):
    CTcpConnection(addr_info, multi_plexer)
{
}

CHttpConnection::~CHttpConnection()
{
}

int CHttpConnection::on_data(char *buf, int size)
{
    fwrite(buf, size, 1, stderr);
    return 0;
}

int CHttpConnection::manage(int sock, int status)
{
    return 0;
}

CHttpConnection *CHttpConnection::instance_from_sock(int sock, IMultiPlexer *multi_plexer)
{
    CHttpConnection *conn = NULL;

    sockaddr_in sock_addr;
    socklen_t sock_len;
    if (getpeername(sock, (sockaddr *)&sock_addr, &sock_len) == 0 && multi_plexer)
    {
        conn = new CHttpConnection;
        if (conn)
        {
            conn->m_sock = sock;
            conn->m_addr_info = sock_addr;
            conn->m_multi_plexer = multi_plexer;
        }
    }

    return conn;
}
