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

#define SERV_PORT 9527
#define OPEN_MAX 1024

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{
    int i, j, n, num = 0;
    int lfd = 0, cfd = 0, sockfd;
    ssize_t nready, efd, res;

    char buf[BUFSIZ];
    char client_ip[INET_ADDRSTRLEN];  //16

    struct epoll_event tep, ep[OPEN_MAX];

    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        sys_err("socket error");
    }

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //端口复用，
    bzero(&serv_addr, sizeof(serv_addr));           //地址结构清零 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(lfd, 128);

    efd = epoll_create(OPEN_MAX);       //创建红黑树，efd指向根节点
    if (efd == -1) {
        sys_err("epoll_create error");
    }

    tep.events = EPOLLIN;
    tep.data.fd = lfd;

    res = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);     //增加节点
    if (res == -1) {
        sys_err("epoll_ctl error");
    }

    while (1)
    {
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);  // -1阻塞监听是否有链接请求
        if (nready < 0) {
            sys_err("epoll_wait error");
        }

        for (i = 0; i < nready; ++i)
        {
            if (!(ep[i].events & EPOLLIN)) {   //不是读事件，继续循环
                continue;
            }
            if (ep[i].data.fd == lfd) {
                clit_addr_len = sizeof(clit_addr);
                cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
                printf("client ip:%s port:%d \n",
                    inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                    ntohs(clit_addr.sin_port));

                tep.events = EPOLLIN;
                tep.data.fd = cfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);  //增加节点
                if (res == -1) {
                    sys_err("epoll_ctl error");
                }
            }
            else {
                sockfd = ep[i].data.fd;
                if ((n = read(sockfd, buf, sizeof(buf))) == 0) {  //client关闭链接
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);  //删除节点
                    if (res == -1) {
                        sys_err("epoll_ctl DEL error");
                    }
                    close(sockfd);
                    printf("client %d closed connection\n", sockfd);
                }
                else if (n < 0)
                {
                    perror("read error: ");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);  //删除节点
                    close(sockfd);
                }
                else       //>0
                {
                    for (j = 0; j < n; j++)
                    {
                        buf[j] = toupper(buf[j]);
                    }
                    write(sockfd, buf, n);
                    write(STDOUT_FILENO, buf, n);
                }
            }
        }
    }
    close(lfd);
    return 0;
}