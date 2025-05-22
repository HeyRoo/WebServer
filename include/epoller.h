/*
 * @Description: epoll 封装
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 17:10:56
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:50:37
 */
#ifndef EPOLLER_H
#define EPOLLER_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    bool addFd(int fd, uint32_t events);

    bool modFd(int fd, uint32_t events);

    bool delFd(int fd);

    int wait(int timeout_ms = -1);

    int getEventFd(size_t i) const;

    uint32_t getEvents(size_t i) const;

private:
    int _epoll_fd;

    std::vector<struct epoll_event> _events;
};

#endif // EPOLLER_H
