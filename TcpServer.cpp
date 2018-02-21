#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "common.h"
#include "Utility.h"
#include "SelectModel.h"
#include "TcpConnection.h"
#include "TcpServer.h"

CTcpServer::CTcpServer():
    m_port(-1),
    m_serv_sock(-1),
    m_multi_plexer(NULL),
    m_factory(NULL)
{
    pthread_mutex_init(&m_mutex, NULL);
}

CTcpServer::CTcpServer(int port, IConnectionFactory *factory, IMultiPlexer *multi_plexer):
    m_port(port),
    m_serv_sock(-1),
    m_multi_plexer(multi_plexer),
    m_factory(factory)
{
    pthread_mutex_init(&m_mutex, NULL);
}

CTcpServer::~CTcpServer()
{
    if (m_serv_sock >= 0)
    {
        close(m_serv_sock);
        m_serv_sock = -1;
    }
}

int CTcpServer::set_unix_sock(int sock)
{
    if (sock < 0)
    {
        LOG_WARN("invalid sock: %d", sock);
        return -1;
    }

    pthread_mutex_lock(&m_mutex);
    m_unix_sock_set.insert(sock);
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

int CTcpServer::remove_unix_sock(int sock)
{
    pthread_mutex_lock(&m_mutex);
    m_unix_sock_set.erase(sock);
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

int CTcpServer::send_client_sock(int sock)
{
    if (sock <= 0 || m_unix_sock_set.size() <= 0)
        return -1;

    int ret = -1;

    pthread_mutex_lock(&m_mutex);
    if (m_unix_sock_set.size() > 0)
    {
        std::set<int>::iterator it = m_unix_sock_set.begin(); 
        for (int idx = CUtility::range_rand(m_unix_sock_set.size()); idx > 0; it++, idx--);
        int s = *it;
        int r = write(s, &sock, sizeof(sock));
        if (r != sizeof(sock))
        {
            close(s);
            m_unix_sock_set.erase(s);
        }
        else
        {
            ret = 0;
        }
    }
    pthread_mutex_unlock(&m_mutex);

    return ret;
}

int CTcpServer::handle_read(IIoHandler **h)
{
    struct sockaddr_in client_address;  
    socklen_t client_len = sizeof(client_address);
    int client_sock = accept(m_serv_sock, (struct sockaddr *)&client_address, &client_len);  
    if (client_sock <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
        {
            LOG_WARN("accept error, sock: %d", m_serv_sock);
        }
        return 0;
    }

    int flags = fcntl(client_sock, F_GETFL, 0);
    fcntl(client_sock, F_SETFL, flags|O_NONBLOCK);

    if (send_client_sock(client_sock) == 0)
        return 0;

    if (h && m_factory)
    {
        *h = m_factory->create(client_sock, TCP_CONN_TYPE_SERVER, m_multi_plexer);
        if (*h)
            return client_sock;
    }

    close(client_sock);
    return 0;
}

int CTcpServer::handle_write(IIoHandler **h)
{
    return -1;
}

int CTcpServer::handle_error()
{
    LOG_WARN("except fd selected: %d", m_serv_sock);
    return 0;
}

int CTcpServer::start(IMultiPlexer *multi_plexer)
{
    if (m_port <= 0)
    {
        LOG_ERROR("invalid port");
        return -1;
    }

    m_multi_plexer = multi_plexer;
    if (!m_multi_plexer)
    {
        LOG_ERROR("multi_plexer null");
        return -1;
    }

    // socket
    ASSIGN_AND_ERROR_ON_NEG(m_serv_sock, socket(AF_INET, SOCK_STREAM, 0));

    // nonblock socket
    int flags = fcntl(m_serv_sock, F_GETFL, 0);
    fcntl(m_serv_sock, F_SETFL, flags|O_NONBLOCK);

    // bind
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));  
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    serv_addr.sin_port = htons(m_port);  

    // set reuse
    int opt = 1;
    setsockopt(m_serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ERROR_ON_NEG(bind(m_serv_sock, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in))); 

    // listen
    ERROR_ON_NEG(listen(m_serv_sock, 1024));

    if (m_multi_plexer->set_read_fd(m_serv_sock, this) != 0) 
        return -1;

    return 0;
}
