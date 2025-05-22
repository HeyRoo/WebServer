/*
 * @Description: 线程池
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-20 17:53:51
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:50:21
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 8);

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool();

    template <class F>
    void addTask(F &&task) {
        {
            std::lock_guard<std::mutex> locker(_pool->mtx);
            _pool->tasks.emplace(std::forward<F>(task));
        }
        _pool->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> _pool;
};

#endif // THREADPOOL_H
