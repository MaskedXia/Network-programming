// 数组优化
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define SERV_PORT 9527

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{

    int i, j, ret, nready, maxi;
    int maxfd = 0;
    int lfd = 0, cfd = 0, sockfd;

    char buf[BUFSIZ];
    char client_ip[INET_ADDRSTRLEN];  //16
    char client[FD_SETSIZE];  //1024

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

    maxfd = lfd;

    maxi = -1;  //记录client下标
    for (i = 0; i < FD_SETSIZE; ++i) {
        client[i] = -1;
    }

    fd_set rset, allset;

    FD_ZERO(&allset);
    FD_SET(lfd, &allset);    //添加监听lfd到集合

    while (1)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);  // 返回满足事件数
        if (nready < 0) {
            sys_err("select error");
        }

        if (FD_ISSET(lfd, &rset)) {                      // 说明有新的客户端请求 lfd=3
            clit_addr_len = sizeof(clit_addr);
            cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
            printf("client ip:%s port:%d \n",
                inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                ntohs(clit_addr.sin_port));

            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = cfd;
                    break;
                }
            }

            if (i == FD_SETSIZE)
            {
                fputs("too many clients\n", stderr);
                exit(1);
            }

            FD_SET(cfd, &allset);

            if (maxfd < cfd)
            {
                maxfd = cfd;
            }

            if (i > maxi)
            {
                maxi = i;
            }

            if (0 == --nready) {        // nready为1，只有lfd有事件，后续for不需要执行
                continue;
            }
        }

        for (i = 0; i <= maxi; ++i)
        {
            if ((sockfd = client[i]) < 0) {
                continue;
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((ret = read(sockfd, buf, sizeof(buf))) == 0) {  //client关闭链接
                    close(sockfd);
                    FD_CLR(i, &allset);
                    client[i] = -1;
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

                if (0 == --nready) {       
                    break;
                }
            }
        }
    }

    close(lfd);

    return 0;
}