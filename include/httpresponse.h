/*
 * @Description: http 响应处理
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:50:29
 */
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "buffer.h"
#include "logger.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string &src_dir, std::string &path, bool is_keep_alive = false, int code = -1);
    void makeResponse(Buffer &buff);
    void unmapFile();
    char *file();
    size_t fileLen() const;
    int code() const { return _code; }

private:
    void _addStateLine(Buffer &buff);
    void _addHeader(Buffer &buff);
    void _addContent(Buffer &buff);
    void _errorContent(Buffer &buff, std::string message);

    void _errorHtml();
    std::string _getFileType();

    int _code;
    bool _is_keep_alive;

    std::string _path;
    std::string _src_dir;

    char *_mm_file;
    struct stat _mm_file_stat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif // HTTP_RESPONSE_H
