/*
 * @Description: 线程池实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-20 17:55:47
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:53:37
 */
#include "threadpool.h"

ThreadPool::ThreadPool(size_t thread_count)
    : _pool(std::make_shared<Pool>()) {
    assert(thread_count > 0);
    for (size_t i = 0; i < thread_count; i++) {
        std::thread([=] {
            std::unique_lock<std::mutex> locker(_pool->mtx);
            while (true) {
                if (!_pool->tasks.empty()) {
                    auto task = std::move(_pool->tasks.front());
                    _pool->tasks.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                } else if (_pool->is_closed)
                    break;
                else
                    _pool->cond.wait(locker);
            }
        }).detach();
    }
}

ThreadPool::~ThreadPool() {
    if (_pool) {
        {
            std::lock_guard<std::mutex> locker(_pool->mtx);
            _pool->is_closed = true;
        }
        _pool->cond.notify_all();
    }
}