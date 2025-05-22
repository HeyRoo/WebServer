/*
 * @Description: 日志类
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 16:14:26
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:48:30
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <assert.h>
#include <mutex>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <thread>

#include "blockqueue.hpp"
#include "buffer.h"

class Logger {
public:
    void init(int level, const char *path = "./log",
              const char *suffix   = ".log",
              int maxQueueCapacity = 1024);

    static Logger *getInstance();
    static void flushLoggerThread();

    void write(int level, const char *format, ...);
    void flush();

    int getLevel();
    void setLevel(int level);
    bool isOpen() { return _is_open; }

private:
    Logger();
    void _appendLoggerLevelTitle(int level);
    virtual ~Logger();
    void _asyncWrite();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES    = 50000;

    const char *_path;
    const char *_suffix;

    int CUR_MAX_LINES;

    int _line_count;
    int _today;
    bool _is_open;
    Buffer _buff;
    int _level;
    bool _is_async;

    FILE *_fp;
    std::unique_ptr<BlockDeque<std::string>> _deque;
    std::unique_ptr<std::thread> _write_thread;
    std::mutex _mtx;
};

#define LOG_BASE(level, format, ...)                           \
    do {                                                       \
        Logger *logger = Logger::getInstance();                \
        if (logger->isOpen() && logger->getLevel() <= level) { \
            logger->write(level, format, ##__VA_ARGS__);       \
            logger->flush();                                   \
        }                                                      \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do {                                   \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do {                                   \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do {                                   \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do {                                   \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);

#endif // LOGGER_H
