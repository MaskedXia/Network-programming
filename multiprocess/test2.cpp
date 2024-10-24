#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
        printf("本程序的程序编号为：%d\n", getpid());
        int ipid = fork();
        sleep(1);
        printf("ipid=%d\n", ipid);
        if(ipid != 0){
                printf("父进程编号为：%d\n", getpid());
        }else{
                printf("子进程编号为：%d\n", getpid());
        }
        sleep(30);
        return 0;
}