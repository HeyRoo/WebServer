cmake_minimum_required(VERSION 3.0)
project(simple_server)

# include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译器版本
set(CMAKE_CXX_STANDARD 11)

# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")


set(SRC_LIST
    src/buffer/buffer.cpp
    src/http/httpconn.cpp
    src/http/httprequest.cpp
    src/http/httpresponse.cpp
    src/logger/logger.cpp
    src/server/epoller.cpp
    src/server/webserver.cpp
    src/thread/threadpool.cpp
    src/timer/timer.cpp
    src/main.cpp
    )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(simple_server ${SRC_LIST})
