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

#define MAXLINE 10

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{
    int efd, i;
    int pfd[2]; 
    pid_t pid;
    char buf[MAXLINE], ch = 'a';

    pipe(pfd);  //管道
    pid = fork();

    if (pid == 0) {    // 子进程 写
        close(pfd[0]);
        while (1) {
            for(i = 0; i < MAXLINE/2; i++){
                buf[i] = ch;
            }
            buf[i - 1] = '\n';
            ch++;
            for(;i < MAXLINE; i++){
                buf[i] = ch;
            }
            buf[i - 1] = '\n';
            // aaaa\nbbbb\n
            write(pfd[1], buf, sizeof(buf));
            sleep(5);
        }
        close(pfd[1]);
    }else if (pid > 0){  // 父进程 读
        close(pfd[1]);

        struct epoll_event ev, events[10];
        int res, len;

        efd = epoll_create(10);
        if (efd == -1)
            sys_err("epoll_create error");
        
        ev.events = EPOLLIN | EPOLLET;  //边沿触发
        ev.data.fd = pfd[0];
        res = epoll_ctl(efd, EPOLL_CTL_ADD, pfd[0], &ev);
        while (1)
        {
            res = epoll_wait(efd, events, 10, -1);
            printf("res %d\n", res);
            if (events[0].data.fd == pfd[0])
            {
                len = read(pfd[0], buf, MAXLINE/2);
                if (len == -1)
                    sys_err("read error");
                write(STDOUT_FILENO, buf, len);
            }
            
        }
        close(pfd[0]);
        close(efd);
    }else{
        sys_err("fork error");
    }
    return 0;
}