#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "TcpServer.h"
#include "SelectModel.h"
#include "ConnectionMakers.h"

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

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, thread_proc, multi_plexer);

    pthread_join(server_thread, NULL);
    return 0;
}
