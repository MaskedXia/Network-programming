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
    int cfd = 0, n;
    char buf[BUFSIZ];

    cfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (cfd == -1) {
        sys_err("socket error");
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);

    while (fgets(buf, BUFSIZ, stdin) != NULL) 
    {

        n = sendto(cfd, buf, strlen(buf), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if (n == -1) 
        {
            sys_err("sendto error");
        }

        n = recvfrom(cfd, buf, BUFSIZ, 0, NULL, 0);  // NULL不关心对端信息
        if (n == -1) 
        {
            sys_err("recvfrom error");
        }
        write(STDOUT_FILENO, buf, n);
    }

    close(cfd);

    return 0;
}