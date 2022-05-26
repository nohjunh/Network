#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char *argv[]){
    int fds[2];
    char str1[] = "Who are you";
    char str2[] = "Thank you for your message";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds);
    pid=fork();

    if(pid==0){
        write(fds[1], str1, sizeof(str1)); // fds[1]은 송신용도 방향
        sleep(2);
        read(fds[0], buf, BUF_SIZE); // fds[0]은 수신용도 방향
        printf("Child proc output: %s \n", buf);
    }
    else{
        read(fds[0], buf, BUF_SIZE); // read는 pipe를 통해 데이터를 읽을때까지 block상태
        printf("Parent proc output: %s \n", buf);
        write(fds[1], str2, sizeof(str2));
        sleep(3);
    }
    return 0;
}