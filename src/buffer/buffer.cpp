/*
 * @Description: buffer 实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-20 18:00:45
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:51:15
 */
#include "buffer.h"

Buffer::Buffer(int init_buffsize)
    : _buffer(init_buffsize)
    , _read_pos(0)
    , _write_pos(0) {}
/**
 * @description: 返回未读数据的长度
 * @return {*}
 */
size_t Buffer::readableBytes() const {
    return _write_pos - _read_pos;
}
/**
 * @description: 返回可写数据的长度
 * @return {*}
 */
size_t Buffer::writableBytes() const {
    return _buffer.size() - _write_pos;
}
/**
 * @description: 返回已读数据长度
 * @return {*}
 */
size_t Buffer::prependableBytes() const {
    return _read_pos;
}
/**
 * @description: 返回已读数据尾部数组地址
 * @return {*}
 */
char *Buffer::beginRead() {
    return _beginPtr() + _read_pos;
}
/**
 * @description: 更新已读数据下标
 * @param {size_t} len
 * @return {*}
 */
void Buffer::hasRead(size_t len) {
    assert(len <= readableBytes());
    _read_pos += len;
}
/**
 * @description: 更新已读数据下标至末尾
 * @param {char} *end
 * @return {*}
 */
void Buffer::hasReadUntil(const char *end) {
    assert(beginRead() <= end);
    hasRead(end - beginRead());
}
/**
 * @description: 清空缓冲区
 * @return {*}
 */
void Buffer::reset() {
    bzero(&_buffer[0], _buffer.size());
    _read_pos  = 0;
    _write_pos = 0;
}
/**
 * @description: 清空缓冲区，并返回内容
 * @return {*}
 */
std::string Buffer::resetToStr() {
    std::string str(beginRead(), readableBytes());
    reset();
    return str;
}
/**
 * @description: 返回已写数据尾部数组地址
 * @return {*}
 */
char *Buffer::beginWrite() {
    return _beginPtr() + _write_pos;
}
/**
 * @description: 更新写数据下标
 * @param {size_t} len
 * @return {*}
 */
void Buffer::hasWritten(size_t len) {
    _write_pos += len;
}
/**
 * @description: 将字符串，加入缓冲区
 * @param {string} &str
 * @return {*}
 */
void Buffer::append(const std::string &str) {
    append(str.data(), str.length());
}
/**
 * @description: 将字符数组，加入缓冲区
 * @param {char} *str
 * @param {size_t} len
 * @return {*}
 */
void Buffer::append(const char *str, size_t len) {
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}
/**
 * @description: 将其他缓冲区数据，加入缓冲区
 * @param {Buffer} &buff
 * @return {*}
 */
void Buffer::append(Buffer &buff) {
    append(buff.beginRead(), buff.readableBytes());
}
/**
 * @description: 检查当前缓冲区是否足够放置数据，否则主动扩容
 * @param {size_t} len
 * @return {*}
 */
void Buffer::ensureWriteable(size_t len) {
    if (writableBytes() < len) {
        _expandBuffer(len);
    }
    assert(writableBytes() >= len);
}
/**
 * @description: 使用readv，从fd中分散读数据到两个buff，并合并
 * @param {int} fd
 * @param {int} *save_errno
 * @return {*}
 */
ssize_t Buffer::readFd(int fd, int *save_errno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t data_size = writableBytes();
    /* 分散读， 保证数据全部读完 */
    /* buff1 */
    iov[0].iov_base = _beginPtr() + _write_pos;
    iov[0].iov_len  = data_size;
    /* buff2 */
    iov[1].iov_base = buff;
    iov[1].iov_len  = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *save_errno = errno;
    } else if (static_cast<size_t>(len) <= data_size) {
        _write_pos += len;
    } else {
        _write_pos = _buffer.size();
        append(buff, len - data_size);
    }
    return len;
}
/**
 * @description: 使用writev，将缓冲区数据一次性写入
 * @param {int} fd
 * @param {int} *save_errno
 * @return {*}
 */
ssize_t Buffer::writeFd(int fd, int *save_errno) {
    size_t data_size = readableBytes();
    ssize_t len      = write(fd, beginRead(), data_size);
    if (len < 0) {
        *save_errno = errno;
        return len;
    }
    _read_pos += len;
    return len;
}
/**
 * @description: 返回缓冲区首地址
 * @return {*}
 */
char *Buffer::_beginPtr() {
    return &*_buffer.begin();
}
/**
 * @description: 回收/扩展缓冲区容量
 * @param {size_t} len
 * @return {*}
 */
void Buffer::_expandBuffer(size_t len) {
    if (_buffer.size() - readableBytes() < len) {
        _buffer.resize(_write_pos + len + 1);
    } else {
        size_t readable = readableBytes();
        std::copy(_beginPtr() + _read_pos, _beginPtr() + _write_pos, _beginPtr());
        _read_pos  = 0;
        _write_pos = readable;
        assert(readable == readableBytes());
    }
}
