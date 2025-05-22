/*
 * @Description: http 连接管理实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:51:39
 */
#include "httpconn.h"

using namespace std;

const char *HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;
bool HttpConn::is_et;

HttpConn::HttpConn()
    : _fd(-1)
    , _addr({0})
    , _is_close(false) {};

HttpConn::~HttpConn() {
    disconn();
};
/**
 * @description: http 连接初始化
 * @param {int} fd
 * @param {sockaddr_in} &addr
 * @return {*}
 */
void HttpConn::init(int fd, const sockaddr_in &addr) {
    assert(fd > 0);
    user_count++;
    _addr = addr;
    _fd   = fd;
    _write_buff.reset();
    _read_buff.reset();
    _is_close = false;
    LOG_INFO("Client[%d](%s:%d) in, user_count:%d", _fd, getIP(), getPort(), (int)user_count);
}
/**
 * @description: 关闭连接，回收内存和打开的文件描述符
 * @return {*}
 */
void HttpConn::disconn() {
    _response.unmapFile();
    if (_is_close == false) {
        _is_close = true;
        user_count--;
        close(_fd);
        LOG_INFO("Client[%d](%s:%d) quit, user_count:%d", _fd, getIP(), getPort(), (int)user_count);
    }
}
/**
 * @description: 获取连接监听的文件描述符
 * @return {*}
 */
int HttpConn::getFd() const {
    return _fd;
};
/**
 * @description: 获取连接监听的socket结构体
 * @return {*}
 */
struct sockaddr_in HttpConn::getAddr() const {
    return _addr;
}
/**
 * @description: 获取连接监听的ip地址
 * @return {*}
 */
const char *HttpConn::getIP() const {
    return inet_ntoa(_addr.sin_addr);
}
/**
 * @description: 获取连接监听的端口
 * @return {*}
 */
int HttpConn::getPort() const {
    return _addr.sin_port;
}
/**
 * @description: 从文件中读数据至读缓冲区，ET模式，只会触发一次回调，应一直读取
 * @param {int} *save_errno
 * @return {*}
 */
ssize_t HttpConn::read(int *save_errno) {
    ssize_t len = -1;
    do {
        len = _read_buff.readFd(_fd, save_errno);
        if (len <= 0) {
            break;
        }
    } while (is_et);
    return len;
}
/**
 * @description: 将缓冲区中的数据，集中写入文件
 * @param {int} *save_errno
 * @return {*}
 */
ssize_t HttpConn::write(int *save_errno) {
    ssize_t len = -1;
    do {
        len = writev(_fd, _iov, _iov_cnt);
        if (len <= 0) {
            *save_errno = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0) {
            break;
        } /* 传输结束 */
        else if (static_cast<size_t>(len) > _iov[0].iov_len) {
            _iov[1].iov_base = (uint8_t *)_iov[1].iov_base + (len - _iov[0].iov_len);
            _iov[1].iov_len -= (len - _iov[0].iov_len);
            if (_iov[0].iov_len) {
                _write_buff.reset();
                _iov[0].iov_len = 0;
            }
        } else {
            _iov[0].iov_base = (uint8_t *)_iov[0].iov_base + len;
            _iov[0].iov_len -= len;
            _write_buff.hasRead(len);
        }
    } while (is_et || toWriteBytes() > 10240);
    return len;
}
/**
 * @description: 客户端连接主处理函数，解析请求信息，返回响应信息
 * @return {*}
 */
bool HttpConn::process() {
    _request.init();
    if (_read_buff.readableBytes() <= 0) {
        return false;
    } else if (_request.parse(_read_buff)) {
        LOG_DEBUG("%s", _request.path().c_str());
        _response.init(src_dir, _request.path(), _request.isKeepAlive(), 200);
    } else {
        _response.init(src_dir, _request.path(), false, 400);
    }

    _response.makeResponse(_write_buff);
    /* 响应头 */
    _iov[0].iov_base = const_cast<char *>(_write_buff.beginRead());
    _iov[0].iov_len  = _write_buff.readableBytes();
    _iov_cnt         = 1;

    /* 文件 */
    if (_response.fileLen() > 0 && _response.file()) {
        _iov[1].iov_base = _response.file();
        _iov[1].iov_len  = _response.fileLen();
        _iov_cnt         = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", _response.fileLen(), _iov_cnt, toWriteBytes());
    return true;
}
