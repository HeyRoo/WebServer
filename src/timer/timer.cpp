/*
 * @Description: 基于时间堆的计时器，使用hash表 + 最小堆(vector，下标从0开始)实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 14:26:42
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:53:54
 */
#include "timer.h"

HeapTimer::HeapTimer() {
    _heap.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}
/**
 * @description: 从下向上调整堆
 * @param {size_t} i
 * @return {*}
 */
void HeapTimer::_siftup(size_t i) {
    assert(i >= 0 && i < _heap.size());
    size_t j = (i - 1) / 2;
    while (i != 0 && j >= 0) {
        if (_heap[j] < _heap[i]) {
            break;
        }
        _swap(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}
/**
 * @description: 交换两个堆节点的位置，包括堆和hash映射表
 * @param {size_t} i
 * @param {size_t} j
 * @return {*}
 */
void HeapTimer::_swap(size_t i, size_t j) {
    assert(i >= 0 && i < _heap.size());
    assert(j >= 0 && j < _heap.size());
    std::swap(_heap[i], _heap[j]);
    _ref_map[_heap[i].id] = i;
    _ref_map[_heap[j].id] = j;
}
/**
 * @description: 从上向下调整堆
 * @param {size_t} index
 * @param {size_t} n，待调整的最小堆的长度
 * @return {*}
 */
bool HeapTimer::_siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < _heap.size());
    assert(n >= 0 && n <= _heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        /* 选择最小的子节点 */
        if (j + 1 < n && _heap[j + 1] < _heap[j])
            j++;
        if (_heap[i] < _heap[j])
            break;
        _swap(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return index < i;
}
/**
 * @description: 添加计时器节点
 * @param {int} id
 * @param {int} timeout 单位 ms
 * @param {TimeoutCallBack} &cb
 * @return {*}
 */
void HeapTimer::add(int id, int timeout, const TimeoutCallBack &cb) {
    assert(id >= 0);
    size_t i;
    if (_ref_map.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i            = _heap.size();
        _ref_map[id] = i;
        _heap.push_back({id, Clock::now() + MS(timeout), cb});
        _siftup(i);
    } else {
        /* 已有结点：调整堆 */
        i                = _ref_map[id];
        _heap[i].expires = Clock::now() + MS(timeout);
        _heap[i].cb      = cb;
        if (!_siftdown(i, _heap.size())) {
            _siftup(i);
        }
    }
}
/**
 * @description: 移除节点并触发回调
 * @param {int} id
 * @return {*}
 */
void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if (_heap.empty() || _ref_map.count(id) == 0) {
        return;
    }
    size_t i       = _ref_map[id];
    TimerNode node = _heap[i];
    node.cb();
    _del(i);
}
/**
 * @description: 删除制定节点，并重新调整堆
 * @param {size_t} index
 * @return {*}
 */
void HeapTimer::_del(size_t index) {
    /* 删除指定位置的结点 */
    assert(!_heap.empty() && index >= 0 && index < _heap.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = _heap.size() - 1;
    assert(i <= n);
    if (i < n) {
        _swap(i, n);
        if (!_siftdown(i, n)) {
            _siftup(i);
        }
    }
    /* 队尾元素删除 */
    _ref_map.erase(_heap.back().id);
    _heap.pop_back();
}
/**
 * @description: 调整指定计时器节点
 * @param {int} id
 * @param {int} timeout
 * @return {*}
 */
void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!_heap.empty() && _ref_map.count(id) > 0);
    _heap[_ref_map[id]].expires = Clock::now() + MS(timeout);
    if (!_siftdown(_ref_map[id], _heap.size())) {
        _siftup(_ref_map[id]);
    }
}
/**
 * @description: 检查时间堆中的任务是否到期
 * @return {*}
 */
void HeapTimer::tick() {
    /* 清除超时结点 */
    if (_heap.empty()) {
        return;
    }
    while (!_heap.empty()) {
        TimerNode node = _heap.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}
/**
 * @description: 移除最小堆的首节点，并更新堆
 * @return {*}
 */
void HeapTimer::pop() {
    assert(!_heap.empty());
    _del(0);
}
/**
 * @description: 清空计时器
 * @return {*}
 */
void HeapTimer::clear() {
    _ref_map.clear();
    _heap.clear();
}
/**
 * @description: 执行已过期计时器的任务，并返回下一个计时器需要等待的时间
 * @return {*}
 */
int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if (!_heap.empty()) {
        res = std::chrono::duration_cast<MS>(_heap.front().expires - Clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}
