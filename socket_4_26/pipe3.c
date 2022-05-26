#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char *argv[]){
    int fds1[2], fds2[2];
    char str1[] = "Who are you";
    char str2[] = "Thank you for your message";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds1), pipe(fds2); // 파이프 2개를 써서 통신하게 되면 제대로 동작한다.
    pid= fork();

    if(pid==0){
        write(fds1[1], str1, sizeof(str1)); //  파이프 fds1([1])의 송신용도 방향으로 write
        read(fds2[0], buf, BUF_SIZE); // 파이프 fds2([0])의 수신용도 방향으로 read 
        printf("Child proc output: %s \n", buf);
    }else{
        read(fds1[0], buf, BUF_SIZE); // 파이프 fds1([0])의 수신용도 방향으로 read
        printf("Parent proc output: %s \n", buf);
        write(fds2[1], str2, sizeof(str2)); // 파이프 fds2([1])의 송신용도 방향으로 write
        sleep(3);
    }
    return 0;
}