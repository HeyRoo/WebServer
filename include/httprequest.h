/*
 * @Description: http 请求处理
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:03:47
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <errno.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "buffer.h"
#include "logger.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer &buff);

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string &key) const;
    std::string getPost(const char *key) const;

    bool isKeepAlive() const;

    /*
    todo
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    bool _parseRequestLine(const std::string &line);
    void _parseHeader(const std::string &line);
    void _parseBody(const std::string &line);

    void _parsePath();
    void _parsePost();
    void _parseFromUrlencoded();

    static bool _userVerify(const std::string &name, const std::string &pwd, bool isLogin);
    static int _converHex(char ch);

    PARSE_STATE _state;
    std::string _method, _path, _version, _body;
    std::unordered_map<std::string, std::string> _header;
    std::unordered_map<std::string, std::string> _post;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif // HTTP_REQUEST_H
