#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <event2/event.h>
#include <sys/stat.h>
#include <fcntl.h>

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

void write_cb(evutil_socket_t fd, short what, void *arg)
{
    char buf[] = "hello libevent";
    int nwrite = write(fd, buf, strlen(buf) + 1);
    if (nwrite == -1) {
        sys_err("write error");
    }
    sleep(1);
}

int main(int argc, char **argv)
{
    // 打开写端
    int fd_write = open("testfifo", O_WRONLY | O_NONBLOCK);
    if (fd_write == -1) {
        sys_err("open write fifo failed");
    }

    // 创建event_base
    struct event_base *base = event_base_new();
    if (!base) {
        sys_err("event_base_new failed");
    }

    // 创建event
    struct event *ev = NULL;
    ev = event_new(base, fd_write, EV_WRITE | EV_PERSIST, write_cb, (void *)base);
    if (!ev) {
        sys_err("event_new failed");
    }

    // 注册事件
    if (event_add(ev, NULL) == -1) {
        sys_err("event_add failed");
    }

    // 启动循环
    event_base_dispatch(base);


    // 释放
    event_free(ev);
    event_base_free(base);
    close(fd_write);


    return 0;
}