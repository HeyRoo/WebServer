/*
 * @Description: server 实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 17:10:56
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:00:00
 */
#include "webserver.h"

using namespace std;

WebServer::WebServer(
        int port, int trig_mode, int timeout_ms, bool opt_linger,
        int thread_num, bool open_log, int log_level, int log_que_size)
    : _port(port)
    , _open_linger(opt_linger)
    , _timeout_ms(timeout_ms)
    , _is_close(false)
    , _timer(new HeapTimer())
    , _threadpool(new ThreadPool(thread_num))
    , _epoller(new Epoller()) {
    _src_dir = getcwd(nullptr, 256);
    assert(_src_dir);
    strncat(_src_dir, "/resources/", 16);
    HttpConn::user_count = 0;
    HttpConn::src_dir    = _src_dir;

    _initEventMode(trig_mode);
    if (!_initSocket()) {
        _is_close = true;
    }

    if (open_log) {
        Logger::getInstance()->init(log_level, "./log", ".log", log_que_size);
        if (_is_close) {
            LOG_ERROR("========== Server init error!==========");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", _port, opt_linger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (_listen_event & EPOLLET ? "ET" : "LT"),
                     (_conn_event & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", log_level);
            LOG_INFO("srcDir: %s", HttpConn::src_dir);
            LOG_INFO("ThreadPool num: %d", thread_num);
        }
    }
}

WebServer::~WebServer() {
    close(_listen_fd);
    _is_close = true;
    free(_src_dir);
}
/**
 * @description: 初始化事件触发模式
 * @param {int} trig_mode
 * @return {*}
 */
void WebServer::_initEventMode(int trig_mode) {
    _listen_event = EPOLLRDHUP;
    _conn_event   = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
    case NO_ET:
        break;
    case CONNECT_ET:
        _conn_event |= EPOLLET;
        break;
    case LISTEN_ET:
        _listen_event |= EPOLLET;
        break;
    case BOTH_ET:
        _listen_event |= EPOLLET;
        _conn_event |= EPOLLET;
        break;
    default:
        _listen_event |= EPOLLET;
        _conn_event |= EPOLLET;
        break;
    }
    HttpConn::is_et = (_conn_event & EPOLLET);
}
/**
 * @description: 服务器主循环
 * @return {*}
 */
void WebServer::start() {
    int time_ms = -1; /* epoll wait timeout == -1 无事件将阻塞 */
    if (!_is_close) {
        LOG_INFO("========== Server start ==========");
    }
    while (!_is_close) {
        /* 消费任务并获取下一计时器间隔时间 */
        if (_timeout_ms > 0) {
            time_ms = _timer->getNextTick();
        }
        int event_cnt = _epoller->wait(time_ms);
        for (int i = 0; i < event_cnt; i++) {
            /* 处理事件 */
            int fd          = _epoller->getEventFd(i);
            uint32_t events = _epoller->getEvents(i);
            /* 情况1：新连接 */
            if (fd == _listen_fd) {
                _dealListen();
            }
            /* 情况2：连接关闭 */
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(_users.count(fd) > 0);
                _closeConn(&_users[fd]);
            }
            /* 情况3：读事件 */
            else if (events & EPOLLIN) {
                assert(_users.count(fd) > 0);
                _dealRead(&_users[fd]);
            }
            /* 情况4：写事件 */
            else if (events & EPOLLOUT) {
                assert(_users.count(fd) > 0);
                _dealWrite(&_users[fd]);
            }
            /* 其他：非预期事件 */
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}
/**
 * @description: 连接异常，发送错误消息并关闭连接
 * @param {int} fd
 * @return {*}
 */
void WebServer::_sendError(int fd, const char *info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}
/**
 * @description: 关闭客户端连接
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_closeConn(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    _epoller->delFd(client->getFd());
    client->disconn();
}
/**
 * @description: 将就绪的文件描述符，添加到监听队列中
 * @param {int} fd
 * @param {sockaddr_in} addr
 * @return {*}
 */
void WebServer::_addClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    _users[fd].init(fd, addr);
    if (_timeout_ms > 0) {
        _timer->add(fd, _timeout_ms, std::bind(&WebServer::_closeConn, this, &_users[fd]));
    }
    _epoller->addFd(fd, EPOLLIN | _conn_event);
    _setFdNonblock(fd);
    LOG_INFO("Client[%d] in!", _users[fd].getFd());
}
/**
 * @description: accepter，接受链接，并增加对应的监听时间
 * @return {*}
 */
void WebServer::_dealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(_listen_fd, (struct sockaddr *)&addr, &len);
        if (fd <= 0) {
            return;
        } else if (HttpConn::user_count >= MAX_FD) {
            _sendError(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        _addClient(fd, addr);
    } while (_listen_event & EPOLLET);
}
/**
 * @description: 读事件处理函数，新增读任务到任务队列
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_dealRead(HttpConn *client) {
    assert(client);
    _extentTime(client);
    _threadpool->addTask(std::bind(&WebServer::_onRead, this, client));
}
/**
 * @description: 写事件处理函数，新增写任务到任务队列
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_dealWrite(HttpConn *client) {
    assert(client);
    _extentTime(client);
    _threadpool->addTask(std::bind(&WebServer::_onWrite, this, client));
}
/**
 * @description: 延长活跃客户端的计时器阻塞时间
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_extentTime(HttpConn *client) {
    assert(client);
    if (_timeout_ms > 0) {
        _timer->adjust(client->getFd(), _timeout_ms);
    }
}
/**
 * @description: 读任务回调函数
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_onRead(HttpConn *client) {
    assert(client);
    int ret        = -1;
    int read_errno = 0;
    ret            = client->read(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) {
        _closeConn(client);
        return;
    }
    _onProcess(client);
}
/**
 * @description: todo
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_onProcess(HttpConn *client) {
    if (client->process()) {
        _epoller->modFd(client->getFd(), _conn_event | EPOLLOUT);
    } else {
        _epoller->modFd(client->getFd(), _conn_event | EPOLLIN);
    }
}
/**
 * @description: 写任务回调函数
 * @param {HttpConn*} client
 * @return {*}
 */
void WebServer::_onWrite(HttpConn *client) {
    assert(client);
    int ret         = -1;
    int write_errno = 0;
    ret             = client->write(&write_errno);
    if (client->toWriteBytes() == 0) {
        /* 传输完成 */
        if (client->isKeepAlive()) {
            _onProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (write_errno == EAGAIN) {
            /* 继续传输 */
            _epoller->modFd(client->getFd(), _conn_event | EPOLLOUT);
            return;
        }
    }
    _closeConn(client);
}
/**
 * @description: 初始化本机监听端口
 * @return {*}
 */
bool WebServer::_initSocket() {
    int ret;
    struct sockaddr_in addr;
    if (_port > 65535 || _port < 1024) {
        LOG_ERROR("Port:%d error!", _port);
        return false;
    }
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(_port);

    struct linger opt_linger = {0};
    if (_open_linger) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        opt_linger.l_onoff  = 1;
        opt_linger.l_linger = 1;
    }
    /* 创建socket */
    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd < 0) {
        LOG_ERROR("Create socket error!", _port);
        return false;
    }
    /* socket 配置 */
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if (ret < 0) {
        close(_listen_fd);
        LOG_ERROR("Init linger error!", _port);
        return false;
    }

    int opt_val = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt_val, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(_listen_fd);
        return false;
    }
    /* 端口绑定 */
    ret = bind(_listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", _port);
        close(_listen_fd);
        return false;
    }
    /* 被动监听，并配置accept队列 */
    ret = listen(_listen_fd, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", _port);
        close(_listen_fd);
        return false;
    }
    /* 新增连接，监听读事件 */
    ret = _epoller->addFd(_listen_fd, _listen_event | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(_listen_fd);
        return false;
    }
    _setFdNonblock(_listen_fd);
    LOG_INFO("Server port:%d", _port);
    return true;
}
/**
 * @description: 设置文件为非阻塞模式，避免accept、read阻塞主线程
 * @param {int} fd
 * @return {*}
 */
int WebServer::_setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
