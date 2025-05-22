/*
 * @Description: logger 实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 16:14:26
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:52:41
 */
#include "logger.h"

using namespace std;

Logger::Logger()
    : _line_count(0)
    , _today(0)
    , _is_open(false)
    , _is_async(false)
    , _fp(nullptr)
    , _deque(nullptr)
    , _write_thread(nullptr) {
}

Logger::~Logger() {
    if (_write_thread && _write_thread->joinable()) {
        while (!_deque->empty()) {
            _deque->flush();
        };
        _deque->close();
        _write_thread->join();
    }
    if (_fp) {
        lock_guard<mutex> locker(_mtx);
        flush();
        fclose(_fp);
    }
}

int Logger::getLevel() {
    lock_guard<mutex> locker(_mtx);
    return _level;
}

void Logger::setLevel(int level) {
    lock_guard<mutex> locker(_mtx);
    _level = level;
}

void Logger::init(int level = 1, const char *path, const char *suffix,
                  int max_queue_size) {
    _is_open = true;
    _level   = level;
    if (max_queue_size > 0) {
        _is_async = true;
        if (!_deque) {
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            _deque = move(newDeque);

            std::unique_ptr<std::thread> NewThread(new thread(flushLoggerThread));
            _write_thread = move(NewThread);
        }
    } else {
        _is_async = false;
    }

    _line_count = 0;

    time_t timer                = time(nullptr);
    struct tm *sysTime          = localtime(&timer);
    struct tm t                 = *sysTime;
    _path                       = path;
    _suffix                     = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             _path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, _suffix);
    _today = t.tm_mday;

    {
        lock_guard<mutex> locker(_mtx);
        _buff.reset();
        if (_fp) {
            flush();
            fclose(_fp);
        }

        _fp = fopen(fileName, "a");
        if (_fp == nullptr) {
            mkdir(_path, 0777);
            _fp = fopen(fileName, "a");
        }
        assert(_fp != nullptr);
    }
}

void Logger::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec        = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t        = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (_today != t.tm_mday || (_line_count && (_line_count % MAX_LINES == 0))) {
        unique_lock<mutex> locker(_mtx);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (_today != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", _path, tail, _suffix);
            _today      = t.tm_mday;
            _line_count = 0;
        } else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", _path, tail, (_line_count / MAX_LINES), _suffix);
        }

        locker.lock();
        flush();
        fclose(_fp);
        _fp = fopen(newFile, "a");
        assert(_fp != nullptr);
    }

    {
        unique_lock<mutex> locker(_mtx);
        _line_count++;
        int n = snprintf(_buff.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

        _buff.hasWritten(n);
        _appendLoggerLevelTitle(level);

        va_start(vaList, format);
        int m = vsnprintf(_buff.beginWrite(), _buff.writableBytes(), format, vaList);
        va_end(vaList);

        _buff.hasWritten(m);
        _buff.append("\n\0", 2);

        if (_is_async && _deque && !_deque->full()) {
            _deque->push_back(_buff.resetToStr());
        } else {
            fputs(_buff.beginRead(), _fp);
        }
        _buff.reset();
    }
}

void Logger::_appendLoggerLevelTitle(int level) {
    switch (level) {
    case 0:
        _buff.append("[debug]: ", 9);
        break;
    case 1:
        _buff.append("[info] : ", 9);
        break;
    case 2:
        _buff.append("[warn] : ", 9);
        break;
    case 3:
        _buff.append("[error]: ", 9);
        break;
    default:
        _buff.append("[info] : ", 9);
        break;
    }
}

void Logger::flush() {
    if (_is_async) {
        _deque->flush();
    }
    fflush(_fp);
}

void Logger::_asyncWrite() {
    string str = "";
    while (_deque->pop(str)) {
        lock_guard<mutex> locker(_mtx);
        fputs(str.c_str(), _fp);
    }
}

Logger *Logger::getInstance() {
    static Logger inst;
    return &inst;
}

void Logger::flushLoggerThread() {
    Logger::getInstance()->_asyncWrite();
}
