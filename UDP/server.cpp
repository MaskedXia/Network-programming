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

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int main(int argc, char* argv[])
{
    char buf[BUFSIZ];
    char client_ip[INET_ADDRSTRLEN];

    int lfd = 0, n;

    lfd = socket(AF_INET, SOCK_DGRAM, 0);  // SOCK_DGRAM UDP
    if (lfd == -1) {
        sys_err("socket error");
    }

    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    printf("Accepting connnections ...\n");
    while (1)
    {
        clit_addr_len = sizeof(clit_addr);
        n = recvfrom(lfd, buf, BUFSIZ, 0, (struct sockaddr*)&clit_addr, &clit_addr_len); 
        if (n == -1) 
        {
            sys_err("recvfrom error");
        }
        printf("client ip:%s port:%d \n",
            inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
            ntohs(clit_addr.sin_port));

        for (int i = 0; i < n; i++)
        {
            buf[i] = toupper(buf[i]);
        }

        n = sendto(lfd, buf, n, 0, (struct sockaddr*)&clit_addr, clit_addr_len);
        if (n == -1) 
        {
            sys_err("sendto error");
        }

    }

    close(lfd);
    return 0;
}