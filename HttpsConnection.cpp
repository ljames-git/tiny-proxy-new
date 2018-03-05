#include "common.h"
#include "HttpsConnection.h"

CHttpsConnection::CHttpsConnection():
    m_ssl_handle(NULL),
    m_ssl_context(NULL)
{
}

CHttpsConnection::CHttpsConnection(sockaddr_in *addr_info, int type, CHttpConnection *manager, IMultiPlexer *multi_plexer):
    CHttpConnection(addr_info, type, manager, multi_plexer),
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
    if (m_conn_state == TCP_CONN_STATE_DISCONNECTED)
        return 0;

    if (m_conn_state == TCP_CONN_STATE_CONNECTED)
        return handshake();

    return 1;
}

int CHttpsConnection::before_write()
{
    if (m_conn_state == TCP_CONN_STATE_DISCONNECTED)
        return 0;

    if (m_conn_state == TCP_CONN_STATE_CONNECTED)
        return handshake();

    return 1;
}

int CHttpsConnection::handshake()
{
    int r = SSL_do_handshake(m_ssl_handle);
    if (r == 1)
    {
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
