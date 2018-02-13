#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "TcpConnection.h"

CTcpConnection::CTcpConnection():
    m_sock(-1),
    m_type(TCP_CONN_TYPE_NONE),
    m_multi_plexer(NULL)
{
    memset(&m_addr_info, 0, sizeof(m_addr_info));
}

CTcpConnection::CTcpConnection(sockaddr_in *addr_info, int type, IMultiPlexer *multi_plexer):
    m_sock(-1),
    m_type(TCP_CONN_TYPE_NONE),
    m_multi_plexer(multi_plexer),
    m_addr_info(*addr_info)
{
    if (type == TCP_CONN_TYPE_SERVER || type == TCP_CONN_TYPE_CLIENT)
        m_type = type;
}

CTcpConnection::~CTcpConnection()
{
    if (m_sock >= 0)
    {
        close(m_sock);
        m_sock = -1;
    }
}

int CTcpConnection::start(IMultiPlexer *multi_plexer)
{
    if (m_type == TCP_CONN_TYPE_NONE)
        return -1;
    return 0;
}

int CTcpConnection::handle_read(IIoHandler **h)
{
    for (;;)
    {
        int nread = read(m_sock, m_read_buf, TCP_READ_BUF_LEN);
        if (nread == 0)
        {
            LOG_DEBUG("sock closed by peer, sock: %d", m_sock);
            return -1;
        }
        if (nread < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                LOG_WARN("read error on sock: %d", m_sock);
                return -1;
            }
            break;
        }

        if (m_type == TCP_CONN_TYPE_SERVER && on_server_data(m_read_buf, nread))
            return -1;

        if (m_type == TCP_CONN_TYPE_CLIENT && on_client_data(m_read_buf, nread))
            return -1;
    }
    return 0;
}

int CTcpConnection::handle_write(IIoHandler **h)
{
    return 0;
}

int CTcpConnection::handle_error()
{
    return 0;
}

int CTcpConnection::on_server_data(char *buf, int size)
{
    return 0;
}

int CTcpConnection::on_client_data(char *buf, int size)
{
    return 0;
}

CTcpConnection *CTcpConnection::instance_from_sock(int sock, int type, IMultiPlexer *multi_plexer)
{
    CTcpConnection *conn = NULL;

    sockaddr_in sock_addr;
    socklen_t sock_len;
    if (getpeername(sock, (sockaddr *)&sock_addr, &sock_len) == 0 && multi_plexer && (type == TCP_CONN_TYPE_SERVER || type == TCP_CONN_TYPE_CLIENT))
    {
        conn = new CTcpConnection;
        if (conn)
        {
            conn->m_sock = sock;
            conn->m_type = type;
            conn->m_addr_info = sock_addr;
            conn->m_multi_plexer = multi_plexer;
        }
    }

    return conn;
}
