#ifndef __REDIS_CONN_POOL_H__
#define __REDIS_CONN_POOL_H__

#include <mutex>
#include <thread>
#include <assert.h>
#include <queue>
#include <semaphore.h>
#include <hiredis/hiredis.h>


class RedisConnPool {
public:
    static RedisConnPool* Instance();

    redisContext* GetConn();
    void FreeConn(redisContext* conn);
    int GetFreeConnCount();

    void Init(const char* host, int port, int connSize = 10);
    void ClosePool();

private:
    RedisConnPool();
    ~RedisConnPool();

    int _MAX_CONN;
    int _useCount;
    int _freeCount;

    std::queue<redisContext*> _connQue;
    std::mutex _mtx;
    sem_t _semId;
};

#endif