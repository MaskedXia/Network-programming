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

void catch_child(int signum)
{   
    while (waitpid(0, NULL, WNOHANG) > 0);
    return;
}

void sys_err(const char* str)
{   
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{

    int lfd = 0, cfd = 0, i;
    pid_t pid;

    int ret;
    char buf[BUFSIZ], client_ip[1024];

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        sys_err("socket error");
    }

    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(lfd, 128);

    clit_addr_len = sizeof(clit_addr);

    while (1)
    {
        cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
        if (cfd == -1) {
            sys_err("accept error");
        }

        pid = fork();    // 创建子进程
        if (pid < -1) {
            sys_err("fork error");
        }
        else if (pid == 0) {  //子进程
            close(lfd);
            break;
        }
        else {  //父进程
            struct sigaction act;  //处理僵尸进程
            act.sa_handler = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;

            ret = sigaction(SIGCHLD, &act, NULL);
            if (ret == -1) {
                sys_err("sigaction error");
            }

            close(cfd);
            continue;
        }

    }

    if (pid == 0)
    {
        while (1)
        {
            ret = read(cfd, buf, sizeof(buf));
            if (ret == 0) {
                close(cfd);
                exit(1);
            }

            write(STDOUT_FILENO, buf, ret);

            for (i = 0; i < ret; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            write(cfd, buf, ret);
        }
    }
    return 0;
}
