#include "redis_conn_pool.h"

RedisConnPool::RedisConnPool() {
    _useCount = 0;
    _freeCount = 0;
}

RedisConnPool::~RedisConnPool() {
    ClosePool();
}

RedisConnPool* RedisConnPool::Instance() {
    static RedisConnPool connPool;
    return &connPool;
}

void RedisConnPool::Init(const char* host, int port, int connSize) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        redisContext* redis = redisConnect(host, port);
        if (redis == NULL || redis->err) {
            if (redis) {
                printf("Error: %s\n", redis->errstr);
            }
            else {
                printf("Can't allocate redis context\n");
            }
            assert(0);
        }
        _connQue.push(redis);
    }
    _MAX_CONN = connSize;
    sem_init(&_semId, 0, _MAX_CONN);
}

redisContext* RedisConnPool::GetConn() {
    redisContext* redis = nullptr;
    if (_connQue.empty()) {
        return nullptr;
    }
    sem_wait(&_semId);
    {
        std::lock_guard<std::mutex> locker(_mtx);
        redis = _connQue.front();
        _connQue.pop();
    }
    return redis;
}

void RedisConnPool::FreeConn(redisContext* redis) {
    assert(redis);
    std::lock_guard<std::mutex> locker(_mtx);
    _connQue.push(redis);
    sem_post(&_semId);
}

void RedisConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(_mtx);
    while (!_connQue.empty()) {
        auto item = _connQue.front();
        _connQue.pop();
        redisFree(item);
    }
}

int RedisConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _connQue.size();
}
