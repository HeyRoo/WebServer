/*
 * @Description: 基于时间堆的计时器
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 14:26:42
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:49:33
 */
#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <chrono>
#include <functional>
#include <queue>
#include <time.h>
#include <unordered_map>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t) {
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();

    ~HeapTimer();

    void adjust(int id, int new_expires);

    void add(int id, int timeout, const TimeoutCallBack &cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int getNextTick();

private:
    void _del(size_t i);

    void _siftup(size_t i);

    bool _siftdown(size_t index, size_t n);

    void _swap(size_t i, size_t j);

    std::vector<TimerNode> _heap;

    std::unordered_map<int, size_t> _ref_map;
};

#endif // TIMER_H
