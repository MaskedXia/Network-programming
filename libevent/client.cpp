#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

//读回调
void read_cb(
    struct bufferevent *bev,
    void *ptr)
{
    char buf[1024];
    int nread = bufferevent_read(bev, buf, sizeof(buf) - 1);
    printf("read from server: %s\n", buf);

    bufferevent_write(bev, buf, strlen(buf) + 1);
    sleep(1);
}

// 写回调
void write_cb(
    struct bufferevent *bev,
    void *ptr)
{
    printf("write_cb function is invoked, actually meaningless\n");
}

// 事件回调
void event_cb(
    struct bufferevent *bev,
    short events,
    void *ptr)
{
    if (events & BEV_EVENT_ERROR) {
        printf("Error from bufferevent!\n");
    } else if (events & BEV_EVENT_EOF) {
        printf("Client close\n");
    } else if (events & BEV_EVENT_TIMEOUT) {
        printf("Timeout!\n");
    } else if (events & BEV_EVENT_CONNECTED) {
        printf("connect to server\n");
        return;
    } else {
        printf("Unknow events %d\n", events);
    }

    bufferevent_free(bev);
    printf("bufferevent is now closed...\n");
}

void read_terminal(evutil_socket_t fd, short events, void *ptr)
{
    char buf[1024];

    int nread = read(fd, buf, sizeof(buf) - 1);
    printf("read from terminal: %s\n", buf);

    struct bufferevent *bev = (struct bufferevent *)ptr;
    bufferevent_write(bev, buf, nread + 1);
}

int main(int argc, char **argv)
{
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9876);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);

    struct event_base *base;
    base = event_base_new();

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        sys_err("socket error");
    }

    struct bufferevent* bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    //连接服务器
    bufferevent_socket_connect(bev, (struct sockaddr*)&serv, sizeof(serv));

    //设置事件回调
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    // 使bufferevent处于可读可写状态并添加到event_base中
    // bufferevent_enable(bev, EV_READ | EV_WRITE);

    // 创建事件
    struct event* ev = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, read_terminal, bev);

    // 注册事件
    event_add(ev, NULL);

    //循环监听
    event_base_dispatch(base);

    // 释放
    event_free(ev);
    event_base_free(base);

    return 0;
}