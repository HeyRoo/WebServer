/*
 * @Description: 主函数
 * @Author: mark
 * @version: 1.0.1
 * @Date: 2025-05-18 17:00:26
 * @LastEditors: Roo
 * @LastEditTime: 2025-05-22 16:02:41
 */
#include "webserver.h"

int main(int argc, char const *argv[])
{
    WebServer server(
        12345, 3, 60000, false,         /*  端口 ET模式 timeout_ms 优雅退出  */
        6, true, 1, 1024);              /*  线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.start();
    return 0;
}
