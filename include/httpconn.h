/*
 * @Description: http 连接管理类
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:03:39
 */
#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "logger.h"
#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in &addr);

    ssize_t read(int *saveErrno);

    ssize_t write(int *saveErrno);

    void disconn();

    int getFd() const;

    int getPort() const;

    const char *getIP() const;

    sockaddr_in getAddr() const;

    bool process();

    int toWriteBytes() {
        return _iov[0].iov_len + _iov[1].iov_len;
    }

    bool isKeepAlive() const {
        return _request.isKeepAlive();
    }

    static bool is_et;
    static const char *src_dir;
    static std::atomic<int> user_count;

private:
    int _fd;
    struct sockaddr_in _addr;

    bool _is_close;

    int _iov_cnt;
    struct iovec _iov[2];

    Buffer _read_buff;  // 读缓冲区
    Buffer _write_buff; // 写缓冲区

    HttpRequest _request;
    HttpResponse _response;
};

#endif // HTTP_CONN_H
