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
#include <sys/stat.h>
#include <fcntl.h>

#define SERV_PORT 9527
#define OPEN_MAX 1024

void sys_err(const char* str)
{
    perror(str);
    exit(1);
}

int get_line(int cfd, char* buf, int len)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < len - 1) && (c != '\n'))    
    {
        n = recv(cfd, &c, 1, 0);
        if (n > 0) {
            if (c == '\r')
            {
                n = recv(cfd, &c, 1, MSG_PEEK);  // MSG_PEEK拷贝读
                if ((n > 0) && (c == '\n')){
                    recv(cfd, &c, 1, 0);
                }else{
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }else{
            c = '\n';
        }
    }
    buf[i] = '\0';
    if(-1 == n){
        i = n;
    }
    return i;
}

void http_request(int sockfd, const char *file)
{
    struct stat sbuf;
    int ret = stat(file, &sbuf);
    if (ret == -1) {
        sys_err("stat error\n");
    }

    if (S_ISREG(sbuf.st_mode)) //是个普通文件
    {
        printf("request file %s\n", file);
        // 回发http应答
        char buf[4096] = {0};
        //sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", sbuf.st_size);
        sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: %d\r\n\r\n", sbuf.st_size);
        send(sockfd, buf, strlen(buf), 0);
        int fd = open(file, O_RDONLY);
        if (fd == -1) {
            sys_err("open error");
        }
        char tmp[1024];
        int n = 0;  // 读取并回发数据
        while ((n = read(fd, tmp, sizeof(tmp))) > 0) {
            send(sockfd, tmp, n, 0);
        }
        close(fd);
    }
    

}

void do_read(int sockfd, int efd){

    char line[1024] = {0};
    int n = get_line(sockfd, line, sizeof(line));
    if(n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            printf("read later\n");
        }
        epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
        close(sockfd);
    }else if(n == 0){
        printf("read over\n");
        epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
        close(sockfd);
    }else{
        char method[16], path[256], protocol[16];
        sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);  //正则  sockfd
        printf("method: %s  path: %s  protocol: %s \n", method, path, protocol);
        while (1)
        {
            char buf[1024] = {0};
            n = get_line(sockfd, buf, sizeof(buf));
            if (n <= 0) {
                break;
            } 
            //printf("request line: %s\n", buf);
        }
        if(strncasecmp(method, "GET", 3) == 0){
            char *file = path + 1; // 取出客户端要访问的文件名
            http_request(sockfd, file);
        }
        
    }
}


int main(int argc, char* argv[])
{

    if (argc < 3) 
    {
        printf("Usage: ./server port path\n"); // ./myhttpd 9527 /root/socketTest/http/txt
    }

    int port = atoi(argv[1]);

    int ret = chdir(argv[2]);
    if (ret != 0) {
        sys_err("chdir error\n");
    }
    

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
    serv_addr.sin_port = htons(port);
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
                do_read(sockfd, efd);
            }
        }
    }
    close(lfd);
    return 0;
}