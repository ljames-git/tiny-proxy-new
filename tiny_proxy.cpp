#include <signal.h>
#include <pthread.h>

#include "Utility.h"
#include "common.h"
#include "TcpServer.h"
#include "SelectModel.h"
#include "ConnectionMakers.h"

#include <errno.h>
#include "HttpConnection.h"

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

    char r[] = "GET / HTTP/1.0\r\nHost: www.163.com\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/6.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/64.0.3282.140 Chrome/64.0.3282.140 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\nAccept-Encoding: deflate\r\nAccept-Language: zh-CN,zh;q=0.9\r\n Cookie: vjuids=37f7922de.161330c90c5.0.5baff2e48c2e4; _ntes_nnid=4a6b849614582687ea541b79148097ac,1516979917035; _ntes_nuid=4a6b849614582687ea541b79148097ac; __gads=ID=21903474cced3712:T=1516979918:S=ALNI_MbtMbaIrumgpFwgjoylA9x5FZwgcw; UM_distinctid=1613aaebf49538-0a41ec77ac6f61-24414032-100200-1613aaebf4a243; P_INFO=lsadw@163.com|1517109022|0|163|11&18|sxi&1516979931&163#sxi&610100#10#0#0|&0|163&mail163|lsadw@163.com; _ga=GA1.2.53235370.1517961774; Province=029; City=029; vjlast=1516979917.1518667893.11; vinfo_n_f_l_n3=15ed57c11050abe6.1.7.1517107961384.1518598020426.1518669236639\r\n\r\n";
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = CUtility::host2randip("www.163.com");
    addr.sin_port = htons(80);
    CHttpConnection *http = new CHttpConnection(&addr, TCP_CONN_TYPE_CLIENT, multi_plexer);
    if (http->start() != 0)
        LOG_INFO("%d\n", errno);
    http->chunk_write(r, sizeof(r));


    pthread_t server_thread;
    pthread_create(&server_thread, NULL, thread_proc, multi_plexer);

    pthread_join(server_thread, NULL);
    return 0;
}
