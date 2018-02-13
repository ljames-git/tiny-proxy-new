#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include <map>
#include <string>

#include "TcpConnection.h"
#include "ConnectionManager.h"

#define HTTP_HEADER_MAX_SIZE 65536
#define HTTP_URI_MAX_SIZE 4096

#define HTTP_METHOD_NONE 0
#define HTTP_METHOD_OPTIONS 1
#define HTTP_METHOD_GET 2
#define HTTP_METHOD_POST 3
#define HTTP_METHOD_CONNECT 4
#define HTTP_METHOD_HEAD 5

class CHttpConnection: public CTcpConnection, public IConnectionManager
{
public:
    CHttpConnection();
    CHttpConnection(sockaddr_in *addr_info, int type, IMultiPlexer *multi_plexer = NULL);
    virtual ~CHttpConnection();

    virtual int manage(int sock, int status);

    static CHttpConnection *instance_from_sock(int sock, int type, IMultiPlexer *multi_plexer);

protected:
    virtual int on_server_data(char *buf, int size);
    virtual int on_client_data(char *buf, int size);

private:
    int parse_req_header();

public:
    std::map<std::string, std::string> m_req_header;

private:
    enum
    {
        RECV_STATE_HEADER = 0,
        RECV_STATE_BODY,
        RECV_STATE_DONE
    };
    int m_recv_state;           // recv state in enum before
    int m_header_size;          // real header size
    int m_header_received;      // bytes received in header
    int m_header_item_offset;   // offset of next header item
    int m_body_offset;          // body offset in header buffer
    int m_method;
    int m_uri_size;
    int m_body_size;
    char *m_body;
    char m_uri[HTTP_URI_MAX_SIZE];
    char m_header_buf[HTTP_HEADER_MAX_SIZE];
};

#endif //__HTTP_CONNECTION_H__
