<!--
 * @Description: readme
 * @Author: Roo
 * @version: 1.0.1
 * @Date: 2025-05-22 16:15:43
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:26:00
-->
# WebServer
C++轻量级高性能WEB服务器

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 基于小根堆实现的定时器，关闭超时的非活动连接；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
* ~~利用hiredis实现了数据库连接池，减少数据库连接建立与关闭的开销；~~


## 测试环境
* Ubuntu 24.04.1 LTS
* C++11

## 目录树
```
.
├── src             源代码目录
│   ├── buffer
│   ├── http
│   ├── log
│   ├── timer
│   ├── pool
│   ├── server
│   └── main.cpp
├── include         头文件目录
├── resources       静态资源
│   ├── index.html
│   ├── image
│   ├── video
│   ├── js
│   └── css
├── simple_server  可执行文件
├── log            日志目录
├── benchmark.txt  压力测试结果
├── CMakeLists.txt
├── LICENSE
└── readme.md
```

## 致谢
此项目fork自[@qinguoyi](https://github.com/markparticle/WebServer)，感谢大佬的开源项目。参考它的最初版本[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)。

