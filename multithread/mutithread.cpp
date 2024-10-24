#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERV_PORT 9527
#define MAXLINE 8192

struct s_info                  //定义一个结构体，将地址结构和cfd捆绑
{
    struct sockaddr_in cliaddr;
    int connfd;
};

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

void* do_work(void* arg)
{
    int ret, i;
    struct s_info* ts = (struct s_info*)arg;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];

    while (1)
    {
        ret = read(ts->connfd, buf, MAXLINE);                   //读客户端
        if (ret < 0)
        {
            sys_err("read error");
        }
        if (ret == 0)
        {
            printf("the client %d closed...\n", ts->connfd);
            break;
        }

        printf("received client ip:%s port:%d \n",
            inet_ntop(AF_INET, &(*ts).cliaddr.sin_addr.s_addr, str, sizeof(str)),
            ntohs((*ts).cliaddr.sin_port));                    //打印客户端信息

        for (i = 0; i < ret; i++)
        {
            buf[i] = toupper(buf[i]);                         // 小写 --> 大写
        }
        write(STDOUT_FILENO, buf, ret);                   //写到屏幕
        write(ts->connfd, buf, ret);                      // 回写客户端
    }
    close(ts->connfd);
    return (void*)0;
}


int main(int argc, char* argv[])
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);
    int lfd, cfd;
    pthread_t tid;

    struct s_info ts[256];
    int i = 0;

    lfd = socket(AF_INET, SOCK_STREAM, 0);     //创建一个socket，得到lfd
    if (lfd == -1) {
        sys_err("socket error");
    }

    bzero(&servaddr, sizeof(servaddr));        //地址结构清零 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(lfd, (struct sockaddr*)&servaddr, sizeof(servaddr));   //绑定

    listen(lfd, 128);

    printf("Accepting client connect ...\n");

    while (1)
    {
        cfd = accept(lfd, (struct sockaddr*)&cliaddr, &cliaddr_len);  //阻塞监听客户端链接请求
        if (cfd == -1) {
            sys_err("accept error");
        }
        ts[i].cliaddr = cliaddr;
        ts[i].connfd = cfd;

        pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
        pthread_detach(tid);                                 //子线程分离，防止僵尸线程
        i++;
    }
    return 0;
}