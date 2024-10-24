#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_PORT 9527


// 小端法：高位存高位，高位存低位 （pc存储）
// 大端法：高位存低位，低位存高位 （网络存储）
// htonl 本地 --> 网络 IP
// htons 本地 --> 网络 port
// ntohl 网络 --> 本地 IP
// ntohs 网络 --> 本地 port

void sys_err(const char* str)
{   
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{
    int cfd = 0;
    int time = 5;
    char buf[BUFSIZ];

    cfd = socket(AF_INET, SOCK_STREAM, 0);

    if (cfd == -1) {
        sys_err("socket error");
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);

    int ret = connect(cfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret == -1) {
        sys_err("connect error");
    }

    while (--time)
    {
        write(cfd, "hello", 5);
        ret = read(cfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, ret);
        sleep(1);
    }

    close(cfd);

    return 0;
}