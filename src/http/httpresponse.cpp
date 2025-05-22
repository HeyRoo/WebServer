/*
 * @Description: http 响应实现
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-21 22:58:19
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 15:52:24
 */
#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse()
    : _code(-1)
    , _is_keep_alive(false)
    , _path("")
    , _src_dir("")
    , _mm_file(nullptr)
    , _mm_file_stat({0}) {};

HttpResponse::~HttpResponse() {
    unmapFile();
}
/**
 * @description: HTTP 应答头初始化
 * @param {string} &src_dir
 * @param {string} &path
 * @param {bool} is_keep_alive
 * @param {int} code
 * @return {*}
 */
void HttpResponse::init(const string &src_dir, string &path, bool is_keep_alive, int code) {
    assert(src_dir != "");
    if (_mm_file) {
        unmapFile();
    }
    _code          = code;
    _is_keep_alive = is_keep_alive;
    _path          = path;
    _src_dir       = src_dir;
    _mm_file       = nullptr;
    _mm_file_stat  = {0};
}
/**
 * @description: 根据缓冲区数据，构造响应头
 * @param {Buffer} &buff
 * @return {*}
 */
void HttpResponse::makeResponse(Buffer &buff) {
    /* 判断请求的资源文件 */
    if (stat((_src_dir + _path).data(), &_mm_file_stat) < 0 || S_ISDIR(_mm_file_stat.st_mode)) {
        _code = 404;
    } else if (!(_mm_file_stat.st_mode & S_IROTH)) {
        _code = 403;
    } else if (_code == -1) {
        _code = 200;
    }
    _errorHtml();
    _addStateLine(buff);
    _addHeader(buff);
    _addContent(buff);
}

char *HttpResponse::file() {
    return _mm_file;
}

size_t HttpResponse::fileLen() const {
    return _mm_file_stat.st_size;
}
/**
 * @description: 检查是否error，并读取对应html资源信息
 * @return {*}
 */
void HttpResponse::_errorHtml() {
    if (CODE_PATH.count(_code) == 1) {
        _path = CODE_PATH.find(_code)->second;
        stat((_src_dir + _path).data(), &_mm_file_stat);
    }
}
/**
 * @description: 构造响应头的响应行
 * @param {Buffer} &buff
 * @return {*}
 */
void HttpResponse::_addStateLine(Buffer &buff) {
    string status;
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        _code  = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + to_string(_code) + " " + status + "\r\n");
}
/**
 * @description: 构造响应头的header键值对
 * @param {Buffer} &buff
 * @return {*}
 */
void HttpResponse::_addHeader(Buffer &buff) {
    buff.append("Connection: ");
    if (_is_keep_alive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + _getFileType() + "\r\n");
}
/**
 * @description: 构造响应头的body
 * @param {Buffer} &buff
 * @return {*}
 */
void HttpResponse::_addContent(Buffer &buff) {
    int src_fd = open((_src_dir + _path).data(), O_RDONLY);
    if (src_fd < 0) {
        _errorContent(buff, "File NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (_src_dir + _path).data());
    int *mmRet = (int *)mmap(0, _mm_file_stat.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (*mmRet == -1) {
        _errorContent(buff, "File NotFound!");
        return;
    }
    _mm_file = (char *)mmRet;
    close(src_fd);
    buff.append("Content-length: " + to_string(_mm_file_stat.st_size) + "\r\n\r\n");
}
/**
 * @description: 解除构造body时，进行的mmap映射
 * @return {*}
 */
void HttpResponse::unmapFile() {
    if (_mm_file) {
        munmap(_mm_file, _mm_file_stat.st_size);
        _mm_file = nullptr;
    }
}
/**
 * @description: 判断文件类型，如果文件路径包含‘.’，或者为指定后缀资源，则认为是明文
 * @return {*}
 */
string HttpResponse::_getFileType() {
    /* 判断文件类型 */
    string::size_type idx = _path.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = _path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}
/**
 * @description: 直接构造a'aerror响应头的body
 * @param {Buffer} &buff
 * @param {string} message
 * @return {*}
 */
void HttpResponse::_errorContent(Buffer &buff, string message) {
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
