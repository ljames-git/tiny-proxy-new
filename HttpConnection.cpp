#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "StringUtil.h"
#include "HttpConnection.h"

CHttpConnection::CHttpConnection():
    m_recv_state(RECV_STATE_HEADER),
    m_header_size(0),
    m_header_received(0),
    m_header_item_offset(0),
    m_body_offset(0),
    m_method(HTTP_METHOD_NONE),
    m_uri_size(0),
    m_body_size(0),
    m_body(NULL)
{
}

CHttpConnection::CHttpConnection(sockaddr_in *addr_info, int type, IMultiPlexer *multi_plexer):
    CTcpConnection(addr_info, type, multi_plexer),
    m_recv_state(RECV_STATE_HEADER),
    m_header_size(0),
    m_header_received(0),
    m_header_item_offset(0),
    m_body_offset(0),
    m_method(HTTP_METHOD_NONE),
    m_uri_size(0),
    m_body_size(0),
    m_body(NULL)
{
}

CHttpConnection::~CHttpConnection()
{
    if (!m_body)
    {
        delete []m_body;
        m_body = NULL;
    }
}

int CHttpConnection::on_server_data(char *buf, int size)
{
    // ingore more data
    if (m_recv_state == RECV_STATE_DONE)
        return 0;

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
            /*
            for (std::map<std::string, std::string>::iterator it = m_req_header.begin(); it != m_req_header.end(); it++)
            {
                LOG_INFO("%s: %s", const_cast<char *>((it->first).c_str()), const_cast<char *>((it->second).c_str()));
            }
            */
        }
    }

    return 0;
}

int CHttpConnection::on_client_data(char *buf, int size)
{
    fprintf(stderr, "%s", buf);
    return 0;
}

int CHttpConnection::manage(int sock, int status)
{
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
                if (strcmp(method, "OPTIONS") == 0) m_method = HTTP_METHOD_OPTIONS;
                else if (strcmp(method, "GET") == 0) m_method = HTTP_METHOD_GET;
                else if (strcmp(method, "POST") == 0) m_method = HTTP_METHOD_POST;
                else if (strcmp(method, "CONNECT") == 0) m_method = HTTP_METHOD_CONNECT;
                else if (strcmp(method, "HEAD") == 0) m_method = HTTP_METHOD_HEAD;
                else
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
