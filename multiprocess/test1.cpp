#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
        printf("本程序的程序编号为：%d\n", getpid());
        sleep(30);
        return 0;
}