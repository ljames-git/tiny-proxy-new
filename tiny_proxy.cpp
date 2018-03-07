#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "TcpServer.h"
#include "SelectModel.h"
#include "ConnectionMakers.h"

#include <errno.h>
#include "Utility.h"
#include "HttpsConnection.h"

#define SERVER_PORT 8888

void *thread_proc(void *arg)
{
    IRunnable *runnable = (IRunnable *)arg;
    for (;;)
    {
        runnable->run();
    }
    return NULL;
}

int main(int argc, char ** argv)
{
    signal(SIGPIPE, SIG_IGN);

    IConnectionFactory *factory = new CHttpConnectionFactory;
    CTcpServer *server = new CTcpServer(SERVER_PORT, factory);
    if (!server)
    {
        LOG_ERROR("create server error");
        return -1;
    }

    IMultiPlexer *multi_plexer = new CSelectModel;
    ERROR_ON_NON_ZERO(server->start(multi_plexer));
    LOG_INFO("SERVER START ON PORT %d", SERVER_PORT);

    /*
    char r[] = "GET / HTTP/1.0\r\nHost: www.baidu.com\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/6.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/64.0.3282.140 Chrome/64.0.3282.140 Safari/537.36\r\n\r\n";
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = CUtility::host2randip("www.baidu.com");
    addr.sin_port = htons(443);
    CHttpsConnection *https = new CHttpsConnection(&addr, TCP_CONN_TYPE_CLIENT, NULL, multi_plexer);
    if (https->start() != 0)
        LOG_INFO("%d\n", errno);
    https->chunk_write(r, sizeof(r));
    */

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, thread_proc, multi_plexer);

    pthread_join(server_thread, NULL);
    return 0;
}
