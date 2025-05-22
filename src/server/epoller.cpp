/*
 * @Description: epoll 封装实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 17:10:56
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:53:01
 */
#include "epoller.h"

Epoller::Epoller(int maxEvent)
    : _epoll_fd(epoll_create(512))
    , _events(maxEvent) {
    assert(_epoll_fd >= 0 && _events.size() > 0);
}

Epoller::~Epoller() {
    close(_epoll_fd);
}
/**
 * @description: 为fd配置epoll，并注册
 * @param {int} fd
 * @param {uint32_t} events
 * @return {*}
 */
bool Epoller::addFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd     = fd;
    ev.events      = events;
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}
/**
 * @description: 修改fd对应配置
 * @param {int} fd
 * @param {uint32_t} events
 * @return {*}
 */
bool Epoller::modFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd     = fd;
    ev.events      = events;
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}
/**
 * @description: 删除fd注册事件
 * @param {int} fd
 * @return {*}
 */
bool Epoller::delFd(int fd) {
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
}
/**
 * @description: epoll事件监听
 * @param {int} timeout_ms
 * @return {*}
 */
int Epoller::wait(int timeout_ms) {
    return epoll_wait(_epoll_fd, &_events[0], static_cast<int>(_events.size()), timeout_ms);
}
/**
 * @description: 从epoll结构体数组中，获取对应事件的文件描述符
 * @param {size_t} i
 * @return {*}
 */
int Epoller::getEventFd(size_t i) const {
    assert(i < _events.size() && i >= 0);
    return _events[i].data.fd;
}
/**
 * @description: 从epoll结构体数组中，获取对应事件的触发类型
 * @param {size_t} i
 * @return {*}
 */
uint32_t Epoller::getEvents(size_t i) const {
    assert(i < _events.size() && i >= 0);
    return _events[i].events;
}
