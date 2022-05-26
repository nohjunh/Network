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

int main(int argc, char *argv[]){
    
    struct iovec vec[2];
    char buf1[]="ABCDEFG";
    char buf2[]="1234567";
    int str_len;

    vec[0].iov_base= buf1; // iov_base는 버퍼의 주소 정보 
    vec[0].iov_len=3; // iov_len은 버퍼의 크기정보
    vec[1].iov_base= buf2;
    vec[1].iov_len=4;

    // writev함수
    // 첫번째 인자는 데이터 전송의 목적지를 나타내는 소켓의 파일디스크립터
    // 두번째 인자는 구조체 iovec 배열의 주소 값 전달
    // 세번째 인자는 두 번째 인자로 전달된 주소 값이 가리키는 배열의 길이정보 전달

    str_len=writev(1, vec, 2); // '표준출력('1'은 stdout)'으로 vec에 있는 data를 출력하는데, vec[0],vec[1] 두 개에 담긴 정보를 str_len에 리턴.

    puts("");
    printf("Write bytes: %d \n", str_len);
    return 0;
}