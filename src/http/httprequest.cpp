/*
 * @Description: http 请求实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:01:33
 */
#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};
/**
 * @description: 请求初始化
 * @return {*}
 */
void HttpRequest::init() {
    _method = _path = _version = _body = "";
    _state                             = REQUEST_LINE;
    _header.clear();
    _post.clear();
}
/**
 * @description: 返回请求头是否正确配置keep-alive
 * @return {*}
 */
bool HttpRequest::isKeepAlive() const {
    if (_header.count("Connection") == 1) {
        return _header.find("Connection")->second == "keep-alive" && _version == "1.1";
    }
    return false;
}
/**
 * @description: 从缓冲区中，解析请求数据
 * @param {Buffer&} buff
 * @return {*}
 */
bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0) {
        return false;
    }
    /* 分段解析：请求行+首部字段+body */
    while (buff.readableBytes() && _state != FINISH) {
        const char *lineEnd = search(buff.beginRead(), buff.beginWrite(), CRLF, CRLF + 2);
        std::string line((const char *)buff.beginRead(), lineEnd);
        switch (_state) {
        case REQUEST_LINE:
            if (!_parseRequestLine(line)) {
                return false;
            }
            _parsePath();
            break;
        case HEADERS:
            _parseHeader(line);
            if (buff.readableBytes() <= 2) {
                _state = FINISH;
            }
            break;
        case BODY:
            _parseBody(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.beginWrite()) {
            break;
        }
        buff.hasReadUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}
/**
 * @description: 解析url中的资源路径
 * @return {*}
 */
void HttpRequest::_parsePath() {
    if (_path == "/") {
        _path = "/index.html";
    } else {
        for (auto &item : DEFAULT_HTML) {
            if (item == _path) {
                _path += ".html";
                break;
            }
        }
    }
}
/**
 * @description: 解析请求行
 * @param {string&} line
 * @return {*}
 */
bool HttpRequest::_parseRequestLine(const string &line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        _method  = subMatch[1];
        _path    = subMatch[2];
        _version = subMatch[3];
        _state   = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}
/**
 * @description: 解析首部键值对
 * @param {string&} line
 * @return {*}
 */
void HttpRequest::_parseHeader(const string &line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        _header[subMatch[1]] = subMatch[2];
    } else {
        _state = BODY;
    }
}
/**
 * @description: 解析body
 * @param {string&} line
 * @return {*}
 */
void HttpRequest::_parseBody(const string &line) {
    _body = line;
    _parsePost();
    _state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}
/**
 * @description: 十六进制数转十进制数
 * @param {char} ch
 * @return {*}
 */
int HttpRequest::_converHex(char ch) {
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}
/**
 * @description: 解析post请求携带的数据
 * @return {*}
 */
void HttpRequest::_parsePost() {
    if (_method == "POST" && _header["Content-Type"] == "application/x-www-form-urlencoded") {
        _parseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(_path)) {
            int tag = DEFAULT_HTML_TAG.find(_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (_userVerify(_post["username"], _post["password"], isLogin)) {
                    _path = "/welcome.html";
                } else {
                    _path = "/error.html";
                }
            }
        }
    }
}
/**
 * @description: 根据post编码格式，解析数据键值对
 * @return {*}
 */
void HttpRequest::_parseFromUrlencoded() {
    if (_body.size() == 0) {
        return;
    }

    string key, value;
    int num = 0;
    int n   = _body.size();
    int i = 0, j = 0;

    for (; i < n; i++) {
        char ch = _body[i];
        switch (ch) {
        case '=':
            key = _body.substr(j, i - j);
            j   = i + 1;
            break;
        case '+':
            _body[i] = ' ';
            break;
        case '%':
            num          = _converHex(_body[i + 1]) * 16 + _converHex(_body[i + 2]);
            _body[i + 2] = num % 10 + '0';
            _body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value      = _body.substr(j, i - j);
            j          = i + 1;
            _post[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (_post.count(key) == 0 && j < i) {
        value      = _body.substr(j, i - j);
        _post[key] = value;
    }
}
/**
 * @description: 验证用户，TODO
 * @param {string} &name
 * @param {string} &pwd
 * @param {bool} isLogin
 * @return {*}
 */
bool HttpRequest::_userVerify(const string &name, const string &pwd, bool isLogin) {
    return true;
}
/**
 * @description: 返回请求资源路径
 * @return {*}
 */
std::string HttpRequest::path() const {
    return _path;
}

std::string &HttpRequest::path() {
    return _path;
}
/**
 * @description: 返回请求方法
 * @return {*}
 */
std::string HttpRequest::method() const {
    return _method;
}
/**
 * @description: 返回请求版本号
 * @return {*}
 */
std::string HttpRequest::version() const {
    return _version;
}
/**
 * @description: 返回post请求数据中，key对应的value
 * @param {string&} key
 * @return {*}
 */
std::string HttpRequest::getPost(const std::string &key) const {
    assert(key != "");
    if (_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}

std::string HttpRequest::getPost(const char *key) const {
    assert(key != nullptr);
    if (_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}
