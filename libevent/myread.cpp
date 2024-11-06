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

void read_cb(evutil_socket_t fd, short what, void *arg)
{
    char buf[1024];
    int nread = read(fd, buf, sizeof(buf) - 1);
    printf("what = %s, read from write: %s\n", 
            what & EV_READ ? "read 满足" : "read不满足", buf);
    if (nread == -1) {
        sys_err("read");
    }
    sleep(1);
}

int main(int argc, char **argv)
{
    // 创建fifo
    unlink("testfifo");
    mkfifo("testfifo", 0644);

    // 打开读端
    int fd_read = open("testfifo", O_RDONLY | O_NONBLOCK);
    if (fd_read == -1) {
        sys_err("open read fifo failed");
    }

    // 创建event_base
    struct event_base *base = event_base_new();
    if (!base) {
        sys_err("event_base_new failed");
    }

    // 创建event
    struct event *ev = NULL;
    ev = event_new(base, fd_read, EV_READ | EV_PERSIST, read_cb, (void *)base);
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
    close(fd_read);


    return 0;
}