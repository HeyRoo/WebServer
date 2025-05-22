/*
 * @Description: 封装readv、writev，实现对文件的统一读写
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-20 18:00:45
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:45:24
 */
#ifndef BUFFER_H
#define BUFFER_H

#include <assert.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

class Buffer {
public:
    Buffer(int init_buffsize = 1024);
    ~Buffer() = default;

    size_t writableBytes() const;

    size_t readableBytes() const;

    size_t prependableBytes() const;

    char *beginRead();

    void ensureWriteable(size_t len);

    void hasWritten(size_t len);

    void hasRead(size_t len);

    void hasReadUntil(const char *end);

    void reset();

    std::string resetToStr();

    char *beginWrite();

    void append(const std::string &str);

    void append(const char *str, size_t len);

    void append(Buffer &buff);

    ssize_t readFd(int fd, int *save_errno);

    ssize_t writeFd(int fd, int *save_errno);

private:
    char *_beginPtr();

    void _expandBuffer(size_t len);

    std::vector<char> _buffer;
    std::atomic<std::size_t> _read_pos;
    std::atomic<std::size_t> _write_pos;
};

#endif // BUFFER_H
