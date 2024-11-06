#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <event2/event.h>

void sys_err(const char *str){
    perror(str);
    exit(1);
}

int main(int argc, char** argv){
    int i;

    struct event_base *base = event_base_new();

    /*支持 poll epoll select*/
    // const char **buf;
    // buf = event_get_supported_methods();
    // for ( i = 0; i < 10; i++)
    // {
    //     printf("buf[%d]=%s\n", i, buf[i]);
    // }

    /*epoll*/
    // const char* buf;
    // buf = event_base_get_method(base);
    // printf("buf=%s\n", buf);

    



    return 0;
    
    
}

