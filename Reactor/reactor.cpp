#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define SERV_PORT 9527
#define BUFLEN 4096
#define MAX_EVENTS 1024

void initListenSocket(int , unsigned short);
void eventset(struct myevent_s* , int , void (*callback)(int, int, void*), void* );
void eventadd(int , int , struct myevent_s* );
void eventdel(int , int , int , void* );
void acceptconn(int , int , void* );
void recvdata(int , int , void* );
void senddata(int , int , void* );


void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

struct myevent_s {
    int fd; //监听的文件描述符          
    int events; //对应的监听事件
    void *arg; //泛型参数
    void (*callback)(int fd, int events, void* arg); //回调函数
    int status;  //是否在监听，1-在红黑树上，0-不在
    char buf[BUFLEN];
    int len;
    long last_active; //记录每次加入红黑树的时间
};

int g_efd;  // 全局变量，保存epoll_create返回的文件描述符
struct myevent_s g_events[MAX_EVENTS+1];  //自定义结构体数组 +1 lfd

// myevent_s初始化
// 设置回调函数  lfd -> acceptconn   cfd -> recvdata
void eventset(struct myevent_s* ev, int fd, void (*callback)(int, int, void*), void* arg)
{
    ev->fd = fd;
    ev->callback = callback;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    memset(ev->buf, 0, sizeof(ev->buf));
    ev->len = 0;
    ev->last_active = time(NULL); //当前时间
}

// 将一个fd添加到红黑树上，设置监听read事件，或者write
void eventadd(int efd, int events, struct myevent_s* ev)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;  // lfd or cfd 
    epv.events = ev->events = events;  // EPOLLIN EPOLLOUT

    if(ev->status == 0){  // 判断是否已经在红黑树上
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if(epoll_ctl(efd, op, ev->fd, &epv) < 0){  // 加到红黑树上
        printf("epoll_ctl error\n");
        return;
    }else{
        printf("eventadd succeed fd=%d op=%d events=%0X\n", ev->fd, op, events);
    }
    
}

void eventdel(int efd, struct myevent_s *ev){
    struct epoll_event epv = {0, {0}};
    if (ev->status != 1)
    {
        return;
    }
    
    epv.data.ptr = NULL;
    ev->status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

// 需要client 接收才行
void senddata(int fd, int events, void* arg)
{
    struct myevent_s* ev = (struct myevent_s*)arg;
    int len;

    len = send(fd, ev->buf, ev->len, 0);

    eventdel(g_efd, ev);
    if(len > 0){
        printf("send[%d]: %s\n", fd, ev->buf);
        eventset(ev, fd, recvdata, ev); // 该cfd设置回调由senddata改为recvdata
        eventadd(g_efd, EPOLLIN, ev);  // 重新加入到红黑树上，等待下一次读取
    }else if(len == 0){  
        printf("send over\n");
        close(ev->fd);
    }else{
        printf("send later\n");
        close(ev->fd);        
    }
}

void recvdata(int fd, int events, void* arg){

    struct myevent_s* ev = (struct myevent_s*)arg;
    int len;

    len = recv(fd, ev->buf, sizeof(ev->buf), 0);

    eventdel(g_efd, ev);  // 将该节点从红黑树上删除

    if(len > 0){
        ev->len = len;
        ev->buf[len] = '\0';
        printf("recv[%d]: %s\n", fd, ev->buf);

        eventset(ev, fd, senddata, ev);  // 该cfd设置回调由recvdata改为senddata
        eventadd(g_efd, EPOLLOUT, ev);  // 监听写事件
    }else if(len == 0){
        printf("[fd=%d] pos[%ld], client close\n", fd, ev-g_events);
        close(ev->fd);
    }else{
        printf("[fd=%d] pos[%ld], error[%d]:%s\n", fd, ev-g_events, errno, strerror(errno));
        close(ev->fd);
    }
}

// 当文件描述符就绪，epoll返回，调用该函数，与客户端建立连接
void acceptconn(int lfd, int events, void* arg){
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int cfd, i;
    cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_len);
    if(cfd == -1){
        if (errno != EAGAIN && errno != EINTR)
        {
            printf("%s: accept new error[%s]\n", __func__, strerror(errno));
        }
        printf("accept error\n");
        return;
    }
    do{
        for (i = 0; i < MAX_EVENTS; i++)  // 全局找出一个空闲元素
        {
            if (g_events[i].status == 0)
            {
                break;
            }   
        }
        if (i == MAX_EVENTS)
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0)  // 设置cfd非阻塞
        {
            printf("%s: fcntl O_NONBLOCK failed[%s]\n", __func__, strerror(errno));
            break;
        }

        eventset(&g_events[i], cfd, recvdata, &g_events[i]);  // cfd回调设置为recvdata

        eventadd(g_efd, EPOLLIN, &g_events[i]);

    }while(0);
    printf("client ip:%s port:%d time:%ld pos:%d\n",
            inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port), g_events[i].last_active, i);
}

void initListenSocket(int g_efd, unsigned short port)
{
    int lfd = 0;
    struct sockaddr_in serv_addr;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        sys_err("socket error"); 
    }
    fcntl(lfd, F_SETFL, O_NONBLOCK);  //设置lfd为非阻塞

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //端口复用，
    bzero(&serv_addr, sizeof(serv_addr));           //地址结构清零 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(lfd, 20);

    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);  // lfd回调设置为acceptconn 数组最后一个元素

    eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);
}


int main(int argc, char* argv[])
{
    unsigned short port = SERV_PORT;
    if(argc == 2){
        port = atoi(argv[1]);
    }

    g_efd = epoll_create(MAX_EVENTS + 1);       //创建红黑树，efd指向根节点
    if (g_efd <= 0) {
        sys_err("epoll_create error");
    }

    initListenSocket(g_efd, port);

    struct epoll_event events[MAX_EVENTS+1];
    printf("server running with port: %d\n", port);

    int checkpos = 0, i;

    while(1){

        // 超时验证，每次测试100个连接，不测试lfd 当客户端60s没有和服务器通信，则关闭客户端连接
        long now = time(NULL);
        for ( i = 0; i < 100; i++)
        {
            if(checkpos == MAX_EVENTS){
                checkpos = 0;
            }
            if (g_events[checkpos].status != 1)
            {
                continue;
            }
            long duration = now - g_events[checkpos].last_active;
            if (duration >= 60)
            {
                close(g_events[checkpos].fd);
                printf("[fd = %d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);
            } 
            checkpos++;
        }

        int nready = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000); //1s没有事件满足，返回0
        if (nready < 0) {
            printf("epoll_wait error, exiting\n");
            break;
        }

        for (i = 0; i < nready; i++)
        {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;

            // 读事件
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
            {
                ev->callback(ev->fd, events[i].events, ev->arg);  // fd, EPOLLIN(EPOLLOUT), arg
            }

            // 写事件
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
            {
                ev->callback(ev->fd, events[i].events, ev->arg);
            }
        }
    }
    return 0;
}