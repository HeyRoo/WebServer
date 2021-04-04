# WebServer
C++轻量级高性能WEB服务器

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 基于小根堆实现的定时器，关闭超时的非活动连接；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
* 利用hiredis实现了数据库连接池，减少数据库连接建立与关闭的开销；


## 环境要求
* Linux
* C++14
* Redis

## 目录树
```
.
├── code           源代码
│   ├── buffer
│   ├── http
│   ├── log
│   ├── timer
│   ├── pool
│   ├── server
│   └── main.cpp
├── resources      静态资源
│   ├── index.html
│   ├── image
│   ├── video
│   ├── js
│   └── css
├── bin            可执行文件
│   └── server
├── log            日志文件
├── webbench-1.5   压力测试
├── build          
│   └── Makefile
├── Makefile
├── LICENSE
└── readme.md
```

## 致谢
此项目fork自[@qinguoyi](https://github.com/markparticle/WebServer)，感谢大佬的开源项目。参考它的最初版本[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)。

