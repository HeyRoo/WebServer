/*
 * @Description: server 对象
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 17:10:56
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:03:55
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "epoller.h"
#include "logger.h"
#include "threadpool.h"
#include "timer.h"
#include "httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trig_mode, int timeout_ms, bool opt_linger,
        int thread_num, bool open_log, int log_level, int log_que_size);

    ~WebServer();
    void start();

    enum TRIGER_MODE {
        NO_ET = 0,
        CONNECT_ET,
        LISTEN_ET,
        BOTH_ET,
    };

private:
    bool _initSocket();
    void _initEventMode(int trigMode);
    void _addClient(int fd, sockaddr_in addr);

    void _dealListen();
    void _dealWrite(HttpConn *client);
    void _dealRead(HttpConn *client);

    void _sendError(int fd, const char *info);
    void _extentTime(HttpConn *client);
    void _closeConn(HttpConn *client);

    void _onRead(HttpConn *client);
    void _onWrite(HttpConn *client);
    void _onProcess(HttpConn *client);

    static const int MAX_FD = 65536;

    static int _setFdNonblock(int fd);

    int _port;
    bool _open_linger;
    int _timeout_ms; /* 毫秒MS */
    bool _is_close;
    int _listen_fd;
    char *_src_dir;

    uint32_t _listen_event;
    uint32_t _conn_event;

    std::unique_ptr<HeapTimer> _timer;
    std::unique_ptr<ThreadPool> _threadpool;
    std::unique_ptr<Epoller> _epoller;
    std::unordered_map<int, HttpConn> _users;
};

#endif // WEBSERVER_H
