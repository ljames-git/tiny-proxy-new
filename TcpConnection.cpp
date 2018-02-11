#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "TcpConnection.h"

CTcpConnection::CTcpConnection():
    m_sock(-1),
    m_multi_plexer(NULL)
{
    memset(&m_addr_info, 0, sizeof(m_addr_info));
}

CTcpConnection::CTcpConnection(sockaddr_in *addr_info, IMultiPlexer *multi_plexer):
    m_sock(-1),
    m_multi_plexer(multi_plexer),
    m_addr_info(*addr_info)
{
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

        if (on_data(m_read_buf, nread) != 0)
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

int CTcpConnection::on_data(char *buf, int size)
{
    return 0;
}

CTcpConnection *CTcpConnection::instance_from_sock(int sock, IMultiPlexer *multi_plexer)
{
    CTcpConnection *conn = NULL;

    sockaddr_in sock_addr;
    socklen_t sock_len;
    if (getpeername(sock, (sockaddr *)&sock_addr, &sock_len) == 0 && multi_plexer)
    {
        conn = new CTcpConnection;
        if (conn)
        {
            conn->m_sock = sock;
            conn->m_addr_info = sock_addr;
            conn->m_multi_plexer = multi_plexer;
        }
    }

    return conn;
}
