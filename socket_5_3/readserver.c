#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/uio.h> // writev, readv

#define BUF_SIZE 1024

int main(int argc, char *argv[]){

    struct iovec vec[2];
    char buf1[BUF_SIZE]={0, };
    char buf2[BUF_SIZE]={0, };
    int str_len;
    
    vec[0].iov_base= buf1;
    vec[0].iov_len=5;
    vec[1].iov_base= buf2;
    vec[1].iov_len= BUF_SIZE; // 버퍼에 있는 값을 다 읽어라.

    str_len= readv(0, vec, 2); // 0:stdin 표준입력 디스크립터
    printf("Read bytes: %d \n", str_len);
    printf("First Message: %s \n", buf1);
    printf("Second Message: %s \n", buf2);
    return 0;
}