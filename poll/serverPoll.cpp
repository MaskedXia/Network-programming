#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#define SERV_PORT 9527
#define OPEN_MAX 1024

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{

    int i, j, ret, nready, maxi;
    int lfd = 0, cfd = 0, sockfd;

    struct pollfd client[OPEN_MAX];

    char buf[BUFSIZ];
    char client_ip[INET_ADDRSTRLEN];  //16

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

    client[0].fd = lfd;
    client[0].events = POLLIN;

    for (i = 1; i < OPEN_MAX; ++i)
    {
        client[i].fd = -1;          //-1初始化，未使用
    }

    maxi = 0;  //记录client数组有效元素最大下标值

    while (1)
    {
        nready = poll(client, maxi+1, -1);  // -1阻塞监听是否有链接请求
        if (nready < 0) {
            sys_err("poll error");
        }

        if (client[0].revents & POLLIN) {
            clit_addr_len = sizeof(clit_addr);
            cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
            printf("client ip:%s port:%d \n",
                inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                ntohs(clit_addr.sin_port));

            for (i = 1; i < OPEN_MAX; ++i) {
                if (client[i].fd < 0) {
                    client[i].fd = cfd;              //找到空闲位置，存放cfd
                    break;
                }
            }

            if (i == OPEN_MAX) {
                sys_err("too many clients\n");
            }

            client[i].events = POLLIN;  //刚刚返回的cfd，监控读事件

            if (i > maxi)
            {
                maxi = i;
            }

            if (0 == --nready) {        // nready为1，只有lfd有事件，后续for不需要执行
                continue;
            }
        }

        for (i = 1; i <= maxi; ++i)
        {
            if ((sockfd = client[i].fd) < 0) {
                continue;
            }
            if (client[i].revents & POLLIN) {
                if ((ret = read(sockfd, buf, sizeof(buf))) == 0) {  //client关闭链接
                    close(sockfd);
                    client[i].fd = -1;
                }
                else if (ret > 0)
                {
                    for (j = 0; j < ret; j++)
                    {
                        buf[j] = toupper(buf[j]);
                    }
                    write(sockfd, buf, ret);
                    write(STDOUT_FILENO, buf, ret);
                }

                if (--nready <= 0) {
                    break;
                }
            }
        }
    }
    close(lfd);
    return 0;
}