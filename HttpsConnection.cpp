#include "common.h"
#include "HttpsConnection.h"

CHttpsConnection::CHttpsConnection():
    m_write_in_read(0),
    m_read_in_write(0),
    m_ssl_handle(NULL),
    m_ssl_context(NULL)
{
}

CHttpsConnection::CHttpsConnection(sockaddr_in *addr_info, int type, CHttpConnection *manager, IMultiPlexer *multi_plexer):
    CHttpConnection(addr_info, type, manager, multi_plexer),
    m_write_in_read(0),
    m_read_in_write(0),
    m_ssl_handle(NULL),
    m_ssl_context(NULL)
{
}

CHttpsConnection::~CHttpsConnection()
{
    if (m_ssl_context)
        SSL_CTX_free(m_ssl_context);

    if (m_ssl_handle)
    {
        SSL_shutdown(m_ssl_handle);
        SSL_free(m_ssl_handle);
    }
}

int CHttpsConnection::on_connected()
{
    SSL_library_init();
    SSL_load_error_strings();

    m_ssl_context = SSL_CTX_new(SSLv23_client_method());
    if (m_ssl_context == NULL)
    {
        LOG_WARN("ssl context initialize failed, sock: %d", m_sock);
        return -1;
    }

    m_ssl_handle = SSL_new(m_ssl_context);
    if (!m_ssl_handle)
    {
        LOG_WARN("failed to create ssl handle, sock: %d", m_sock);
        return -1;
    }

    if (!SSL_set_fd(m_ssl_handle, m_sock)) 
    {
        LOG_WARN("ssl set fd error, sock: %d", m_sock);
        return -1;
    }

    SSL_set_connect_state(m_ssl_handle);

    return handshake();
}

int CHttpsConnection::before_read()
{
    if (CTcpConnection::before_read() < 0)
        return -1;

    if (m_conn_state == TCP_CONN_STATE_DISCONNECTED)
        return 0;

    if (m_conn_state == TCP_CONN_STATE_CONNECTED)
        return handshake();
    
    return 0;
}

int CHttpsConnection::handle_read(IIoHandler **h)
{
    if (m_write_in_read && do_ssl_write() < 0)
        return -1;

    return do_ssl_read();
}

int CHttpsConnection::before_write()
{
    if (CTcpConnection::before_write() < 0)
        return -1;

    if (m_conn_state == TCP_CONN_STATE_DISCONNECTED)
        return 0;

    if (m_conn_state == TCP_CONN_STATE_CONNECTED)
        return handshake();

    return 0;
}

int CHttpsConnection::handle_write(IIoHandler **h)
{
    if (m_read_in_write && do_ssl_read() < 0)
        return -1;

    return do_ssl_write();
}

int CHttpsConnection::handshake()
{
    int r = SSL_do_handshake(m_ssl_handle);
    if (r == 1)
    {
        m_multi_plexer->set_read_fd(m_sock, this);
        m_multi_plexer->set_write_fd(m_sock, this);
        m_conn_state = HTTPS_CONN_CONNECTED;
        return 0;
    }

    int err = SSL_get_error(m_ssl_handle, r);
    if (err == SSL_ERROR_WANT_WRITE)
    {
        m_multi_plexer->clear_fd(m_sock);
        m_multi_plexer->set_write_fd(m_sock, this);
    }
    else if (err == SSL_ERROR_WANT_READ)
    {
        m_multi_plexer->clear_fd(m_sock);
        m_multi_plexer->set_read_fd(m_sock, this);
    }
    else
    {
        LOG_WARN("ssl hand shake error, sock: %d", m_sock);
        return -1;
    }

    return 1;
}

int CHttpsConnection::do_ssl_read()
{
    for (;;)
    {
        int nread = SSL_read(m_ssl_handle, m_read_buf, TCP_READ_BUF_LEN);
        if (nread == 0)
        {
            LOG_DEBUG("https sock closed by peer, sock: %d", m_sock);
            return -1;
        }
        if (nread < 0)
        {
            switch (SSL_get_error(m_ssl_handle, nread))
            {
            case SSL_ERROR_WANT_READ:
                m_write_in_read = 0;
                break;
            case SSL_ERROR_WANT_WRITE:
                m_write_in_read = 1;
                break;
            default:
                LOG_DEBUG("https read error, sock: %d", m_sock);
                return -1;
            }
        }

        if (m_type == TCP_CONN_TYPE_SERVER && on_server_data(m_read_buf, nread))
            return -1;

        if (m_type == TCP_CONN_TYPE_CLIENT && on_client_data(m_read_buf, nread))
            return -1;
    }

    return 0;
}

int CHttpsConnection::do_ssl_write()
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
            int nwrite = SSL_write(m_ssl_handle, (char *)chunk_data->m_data + start, to_write);
            if (nwrite == 0)
            {
                LOG_DEBUG("https sock closed by peer, sock: %d", m_sock);
                return -1;
            }

            if (nwrite < 0)
            {
                switch (SSL_get_error(m_ssl_handle, nwrite))
                {
                    case SSL_ERROR_WANT_READ:
                        m_read_in_write = 1;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        m_read_in_write = 0;
                        break;
                    default:
                        LOG_DEBUG("https write error, sock: %d", m_sock);
                        return -1;
                }
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
