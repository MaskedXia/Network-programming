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
#define OPEN_MAX 1024

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{
    int i, j, n, num = 0;
    int lfd = 0, cfd = 0, sockfd, flag;
    size_t nready, efd, res;

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

    //tep.events = EPOLLIN;  // 水平触发，默认
    tep.events = EPOLLIN | EPOLLET; // ET边沿触发
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

                //tep.events = EPOLLIN;
                tep.events = EPOLLIN | EPOLLET;
                tep.data.fd = cfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);  //增加节点
                if (res == -1) {
                    sys_err("epoll_ctl error");
                }

                // 修改cfd为非阻塞
                flag = fcntl(cfd, F_GETFL);  
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);
            }
            else {
                printf("event triggered once\n");
                sockfd = ep[i].data.fd;
                while(1){  //非阻塞读，轮询读
                    n = read(sockfd, buf, 5);
                    if(n < 0){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            printf("read later\n");
                            break;
                        }
                        close(sockfd);
                        break;
                    }else if(n == 0){
                        printf("read over\n");
                        close(sockfd);
                    }else{
                        for (j = 0; j < 5; j++)
                        {
                            buf[j] = toupper(buf[j]);
                        }
                        write(STDOUT_FILENO, buf, n);
                    }

                }
            }
        }
    }
    close(lfd);
    return 0;
}