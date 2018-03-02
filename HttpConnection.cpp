#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "Utility.h"
#include "StringUtil.h"
#include "HttpConnection.h"

const char *CHttpConnection::m_method_map[HTTP_MEHTOD_SIZE] = 
{
    "",
    "OPTIONS",
    "GET",
    "POST",
    "CONNECT",
    "HEAD"
};

CHttpConnection::CHttpConnection():
    m_recv_state(RECV_STATE_HEADER),
    m_header_size(0),
    m_header_received(0),
    m_header_item_offset(0),
    m_body_offset(0),
    m_method(HTTP_METHOD_NONE),
    m_body_size(0),
    m_parent_connection(NULL),
    m_child_connection(NULL),
    m_body(NULL)
{
}

CHttpConnection::CHttpConnection(sockaddr_in *addr_info, int type, CHttpConnection *manager, IMultiPlexer *multi_plexer):
    CTcpConnection(addr_info, type, multi_plexer),
    m_recv_state(RECV_STATE_HEADER),
    m_header_size(0),
    m_header_received(0),
    m_header_item_offset(0),
    m_body_offset(0),
    m_method(HTTP_METHOD_NONE),
    m_body_size(0),
    m_parent_connection(manager),
    m_child_connection(NULL),
    m_body(NULL)
{
}

CHttpConnection::~CHttpConnection()
{
    if (m_parent_connection)
        m_parent_connection->manage(get_sock(), 0);

    if (m_child_connection)
        delete m_child_connection;

    if (!m_body)
    {
        delete []m_body;
        m_body = NULL;
    }
}

int CHttpConnection::server_req_done()
{
    std::map<std::string, std::string>::iterator iter = m_req_header.find("Host");
    if (iter == m_req_header.end())
    {
        LOG_WARN("no host field, uri: %s", m_uri);
        return -1;
    }

    int port = 80;
    std::string raw_host = m_req_header["Host"];
    const char *host = raw_host.c_str();
    size_t pos = raw_host.find(':');
    if (pos != std::string::npos)
    {
        host = raw_host.substr(0, pos).c_str();
        port = atoi(raw_host.substr(pos).c_str());
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = CUtility::host2randip(host);
    CHttpConnection *http_client = new CHttpConnection(&addr, TCP_CONN_TYPE_CLIENT, this);
    if (!http_client)
    {
        LOG_WARN("new http_client error, not enough memory, uri: %s", m_uri);
        return -1;
    }

    m_child_connection = http_client;

    // write request first line
    char line[HTTP_HEADER_MAX_SIZE];
    const char *req_version = "HTTP/1.1";
    snprintf(line, sizeof(line), "%s %s %s\r\n", m_method_map[m_method], m_uri, req_version);
    if (http_client->chunk_write(line, strlen(line)) != 0)
    {
        LOG_WARN("chunk write error, line:%s, uri: %s", line, m_uri);
        delete http_client;
        return -1;
    }

    // write other header lines
    for (std::map<std::string, std::string>::iterator it = m_req_header.begin(); it != m_req_header.end(); it++)
    {
        if (it->first == "Connection" || it->first == "Proxy-Connection")
            continue;

        snprintf(line, sizeof(line), "%s: %s\r\n", (it->first).c_str(), (it->second).c_str());
        if (http_client->chunk_write(line, strlen(line)) != 0)
        {
            LOG_WARN("chunk write error, line:%s, uri: %s", line, m_uri);
            delete http_client;
            return -1;
        }
    }

    // write the end of header
    char header_end[] = "\r\n";
    if (http_client->chunk_write(header_end, strlen(header_end)) != 0)
    {
        LOG_WARN("chunk write error, end, uri: %s", m_uri);
        delete http_client;
        return -1;
    }

    // write body if exists
    if (m_body && m_body_size > 0 && http_client->chunk_write(m_body, m_body_size) != 0)
    {
        LOG_WARN("chunk write error, body, uri: %s", m_uri);
        delete http_client;
        return -1;
    }

    if (http_client->start(m_multi_plexer) != 0)
    {
        LOG_WARN("http client start error, errno: %d, host: %s, port: %d uri: %s", errno, host, port, m_uri);
        delete http_client;
        return -1;
    }

    return 0;
}

int CHttpConnection::on_server_data(char *buf, int size)
{
    // ingore more data
    if (m_recv_state == RECV_STATE_DONE)
    {
        //LOG_INFO("still trans after done, %s", buf);
        return -1;
    }

    if (m_recv_state == RECV_STATE_HEADER)
    {
        int copy_size = size > HTTP_HEADER_MAX_SIZE - m_header_received ? HTTP_HEADER_MAX_SIZE - m_header_received : size;
        memcpy(m_header_buf + m_header_received, buf, copy_size);
        m_header_received += copy_size;
        if (parse_req_header() < 0)
        {
            LOG_WARN("parse request error");
            return -1;
        }
    }

    if (m_recv_state == RECV_STATE_BODY)
    {
        int content_length = 0;
        if (m_req_header.find("Content-Length") != m_req_header.end())
        {
            content_length = atoi(m_req_header["Content-Length"].c_str());
            if (content_length < 0)
            {   
                LOG_WARN("content length invalid: %d, uri: %s", content_length, m_uri);
                return -1; 
            }
        }

        if (!m_body)
        {
            m_body = new char[content_length];
            if (!m_body)
            {
                LOG_WARN("alloc body memory error");
                return -1;
            }

            // copy body data from header buf if any
            int copy_size = m_header_received - m_body_offset;
            if (copy_size > 0)
            {
                memcpy(m_body, m_header_buf + m_body_offset, copy_size);
                m_body_size += copy_size;
            }
        }
        else
        {
            int copy_size = size;
            if (m_body_size + copy_size >= content_length)
                copy_size = content_length - m_body_size;

            memcpy(m_body, buf, copy_size);
            m_body_size += copy_size;
        }

        // request done
        if (m_body_size >= content_length)
        {
            m_recv_state = RECV_STATE_DONE;
            return server_req_done();
        }
    }

    return 0;
}

int CHttpConnection::on_client_data(char *buf, int size)
{
    /*
    char *b = new char[size + 1];
    memcpy(b, buf, size);
    b[size] = 0;
    fprintf(stderr, "%s", b);
    delete []b;
    */

    if (m_parent_connection && m_parent_connection->chunk_write(buf, size) != 0)
        return -1;

    return 0;
}

int CHttpConnection::manage(int sock, int status)
{
    if (m_child_connection && m_child_connection->get_sock() == sock)
    {
        m_child_connection = NULL;
    }

    return 0;
}

int CHttpConnection::parse_req_header()
{
    for (char *p = m_header_buf + m_header_size, *q = m_header_buf + m_header_item_offset;
            m_header_size < m_header_received && m_header_size < HTTP_HEADER_MAX_SIZE;
            p++, m_header_size++)
    {
        if (*p == '\n' && p != m_header_buf && *(p - 1) == '\r')
        {
            *(p - 1) = 0;
            if (strlen(q) == 0)
            {
                // header end
                m_body_offset = ++(m_header_size);
                m_recv_state = RECV_STATE_BODY;
                return 0;
            }

            if (m_method == HTTP_METHOD_NONE)
            {
                // parse first line
                // [method] [path] HTTP/x.x

                char *t = q;

                // parse method
                for (; *t && *t != ' '; t++);
                if (!*t)
                    return -1;
                *t = 0;
                char *method = CStringUtil::trim(q);
                for (int i = 0; i < HTTP_MEHTOD_SIZE; i++)
                {
                    if (strcmp(method, m_method_map[i]) == 0)
                    {
                        m_method = i;
                        break;
                    }
                }
                if (m_method == HTTP_METHOD_NONE)
                {
                    LOG_WARN("unsupported method: %s", method);
                    return -1;
                }

                // parse uri/path
                char *r = CStringUtil::trim(t + 1);
                for (t = r; *t && *t != ' '; t++);
                *t = 0;
                char *uri = CStringUtil::trim(r);
                snprintf(m_uri, sizeof(m_uri), "%s", uri);
            }
            else
            {
                // parse other header items
                char *t = q;
                for (; *t && *t != ':'; t++);
                if (*t == ':')
                {
                    *t = 0;
                    const char *key = CStringUtil::trim(q);
                    const char *value = CStringUtil::trim(t + 1);
                    if (m_type == TCP_CONN_TYPE_SERVER)
                    {
                        if (m_req_header.find(key) == m_req_header.end())
                            m_req_header[key] = value;
                    }
                }
            }

            q = p + 1;
            m_header_item_offset = q - m_header_buf;
        }
    }

    if (m_header_size >= HTTP_HEADER_MAX_SIZE)
    {
        // header is too long
        LOG_WARN("header is too long, size: %d, uri: %s", m_header_size, m_uri);
        return -1;
    }

    return 0;
}

CHttpConnection *CHttpConnection::instance_from_sock(int sock, int type, IMultiPlexer *multi_plexer)
{
    CHttpConnection *conn = NULL;

    sockaddr_in sock_addr;
    socklen_t sock_len;
    if (getpeername(sock, (sockaddr *)&sock_addr, &sock_len) == 0 && multi_plexer && (type == TCP_CONN_TYPE_SERVER || type == TCP_CONN_TYPE_CLIENT))
    {
        conn = new CHttpConnection;
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
