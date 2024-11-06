#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
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
    if (nread <= 0) {
        printf("client close\n");
        bufferevent_free(bev);
        return;
    }
    buf[nread] = '\0';
    printf("read from client: %s\n", buf);

    // 写数据给客户端
    char *p = "I am server, I have received your message, thank you!\n";
    bufferevent_write(bev, p, strlen(p) + 1);
    sleep(1);
}

// 写回调
void write_cb(
    struct bufferevent *bev,
    void *ptr)
{
    printf("I am server, successfuly write data to client, write_cb function is invoked\n");
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
    } else {
        printf("Unknow events %d\n", events);
    }

    bufferevent_free(bev);
    printf("bufferevent is now closed...\n");
}

void cb_listener(
    struct evconnlistener *listener,
    evutil_socket_t sock,
    struct sockaddr* addr,
    int len,
    void* ptr)
{
    printf("connect new client\n");

    // 创建bufferevent并添加到event_base中
    struct event_base *base = (event_base *)ptr;
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    // 设置事件回调
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    // 使bufferevent处于可读可写状态并添加到event_base中
    bufferevent_enable(bev, EV_READ | EV_WRITE);

}

int main(int argc, char **argv)
{
    struct sockaddr_in serv;

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9876);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    struct event_base *base;
    base = event_base_new();

    struct evconnlistener* listener;
    listener = evconnlistener_new_bind(base, cb_listener, (void *)base,
            LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
            (struct sockaddr*)&serv, sizeof(serv));
    if (!listener) {
        sys_err("Could not create a listener!");
    }

    //循环监听
    event_base_dispatch(base);

    // 释放
    evconnlistener_free(listener);
    event_base_free(base);
    return 0;
}