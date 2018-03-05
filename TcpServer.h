#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <set>
#include <pthread.h>
#include "IoHandler.h"
#include "MultiPlexer.h"
#include "ConnectionFactory.h"

#define MAX_UNIX_SOCKS 32

class CTcpServer: public IIoHandler
{
public:
    CTcpServer();
    CTcpServer(int port, IConnectionFactory *factory = NULL, IMultiPlexer *multi_plexer = NULL);
    virtual ~CTcpServer();

    int set_unix_sock(int sock);
    int remove_unix_sock(int sock);
    int send_client_sock(int sock);
    int start(IMultiPlexer *multi_plexer = NULL);

public:
    // implementation of IIoHandler
    
    // > 0: OK, but must not call handle_read
    // == 0: OK, continue to handle_read
    // < 0: error
    virtual int before_read();

    // > 0: OK, but must not call handle_write
    // == 0: OK, continue to handle_write
    // < 0: error
    virtual int before_write();

    // 0: OK, other: remove sock
    virtual int handle_read(IIoHandler **h = NULL);
    
    // 0: OK, other: remove sock
    virtual int handle_write(IIoHandler **h = NULL);

    virtual int handle_error();

private:
    int m_port;
    int m_serv_sock;
    IMultiPlexer *m_multi_plexer;
    IConnectionFactory *m_factory;
    pthread_mutex_t m_mutex;
    std::set<int> m_unix_sock_set;
};

#endif //__TCP_SERVER_H__
