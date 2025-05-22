/*
 * @Description: 生产者/消费者队列模版
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 16:14:26
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:50:42
 */
#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <sys/time.h>

template <class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t max_capacity = 1000);

    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    void close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T &item);

    void push_front(const T &item);

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();

private:
    /* 临界区生产队列 */
    std::deque<T> _deq;
    /* 最大容量 */
    size_t _capacity;
    /* 临界区互斥锁 */
    std::mutex _mtx;
    /* 生产队列状态 */
    bool _is_close;
    /* 消费者 */
    std::condition_variable _cond_consumer;
    /* 生产者 */
    std::condition_variable _cond_producer;
};

template <class T>
BlockDeque<T>::BlockDeque(size_t max_capacity)
    : _capacity(max_capacity) {
    assert(max_capacity > 0);
    _is_close = false;
}

template <class T>
BlockDeque<T>::~BlockDeque() {
    close();
};

template <class T>
void BlockDeque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(_mtx);
        _deq.clear();
        _is_close = true;
    }
    _cond_producer.notify_all();
    _cond_consumer.notify_all();
};

template <class T>
void BlockDeque<T>::flush() {
    _cond_consumer.notify_one();
};

template <class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(_mtx);
    _deq.clear();
}

template <class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.front();
}

template <class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.back();
}

template <class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size();
}

template <class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _capacity;
}

template <class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.size() >= _capacity) {
        _cond_producer.wait(locker);
    }
    _deq.push_back(item);
    _cond_consumer.notify_one();
}

template <class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.size() >= _capacity) {
        _cond_producer.wait(locker);
    }
    _deq.push_front(item);
    _cond_consumer.notify_one();
}

template <class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.empty();
}

template <class T>
bool BlockDeque<T>::full() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size() >= _capacity;
}

template <class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.empty()) {
        _cond_consumer.wait(locker);
        if (_is_close) {
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _cond_producer.notify_one();
    return true;
}

template <class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.empty()) {
        if (_cond_consumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            return false;
        }
        if (_is_close) {
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _cond_producer.notify_one();
    return true;
}

#endif // BLOCKQUEUE_H
