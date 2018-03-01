#include <errno.h>
#include <fcntl.h>
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
        if (m_multi_plexer)
        {
            m_multi_plexer->clear_fd(m_sock);
        }

        close(m_sock);
        m_sock = -1;
    }

    if (m_chunk_queue.size() > 0)
    {
        delete(m_chunk_queue.front());
        m_chunk_queue.pop();
    }
}

int CTcpConnection::start(IMultiPlexer *multi_plexer)
{
    if (multi_plexer)
        m_multi_plexer = multi_plexer;

    if (m_type != TCP_CONN_TYPE_CLIENT || !m_multi_plexer || m_sock != -1)
        return -1;

    // socket
    ASSIGN_AND_ERROR_ON_NEG(m_sock, socket(AF_INET, SOCK_STREAM, 0));

    // nonblock socket
    int flags = fcntl(m_sock, F_GETFL, 0);
    fcntl(m_sock, F_SETFL, flags|O_NONBLOCK);

    if (connect(m_sock, (const sockaddr *)&m_addr_info, sizeof(m_addr_info)) < 0 && errno != EINPROGRESS)
        return -1;

    m_multi_plexer->set_write_fd(m_sock, this);
    m_multi_plexer->set_read_fd(m_sock, this);

    return 0;
}

int CTcpConnection::chunk_write(void *data, int size, int last)
{
    if (!data || size <= 0)
        return -1;

    //fwrite(data, size, 1, stderr);
    //return 0;

    CChunkData *chunk_data = new CChunkData(data, size, last);
    m_chunk_queue.push(chunk_data);

    if (m_sock >= 0 && m_multi_plexer)
        m_multi_plexer->set_write_fd(m_sock, this);

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
    if (m_chunk_queue.size() <= 0)
    {
        m_multi_plexer->clear_write_fd(m_sock);
        return 0;
    }

    for (CChunkData *chunk_data = m_chunk_queue.front(); chunk_data; chunk_data = m_chunk_queue.front())
    {
        for (int start = chunk_data->m_offset, to_write = chunk_data->m_size - chunk_data->m_offset;
             to_write > 0;
             start = chunk_data->m_offset, to_write = chunk_data->m_size - chunk_data->m_offset)
        {
            int nwrite = write(m_sock, (char *)chunk_data->m_data + start, to_write);
            if (nwrite < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                {
                    LOG_WARN("write error on sock: %d", m_sock);
                    return -1;
                }
                return 0;
            }

            chunk_data->m_offset += nwrite;
        }

        int last = chunk_data->m_last;
        delete chunk_data;

        m_chunk_queue.pop();
        if (m_chunk_queue.size() <= 0)
        {
            if (last)
                write_done();

            m_multi_plexer->clear_write_fd(m_sock);
            break;
        }
    }

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

int CTcpConnection::write_done()
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
