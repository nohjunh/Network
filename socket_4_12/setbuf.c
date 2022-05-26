#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> //도메인관련 함수 쓰려면 관련 헤더파일 선언

int main(int argc, char** argv){
    int sock;
    int snd_buf, rcv_buf;
    socklen_t len;
    int state;

    snd_buf = rcv_buf = 1024*3; // 설정한 값 그대로 버퍼 크기가 결정되지 않고 커널 정책에 따라 결정됨
    
    sock= socket(AF_INET, SOCK_STREAM, 0);
    state = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, sizeof(rcv_buf));
    if(state){
        perror("setsockopt() error");
        return 1;
    }

    state = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, sizeof(snd_buf));
    if(state){
        perror("setsockopt() error");
        return 1;
    }

    len= sizeof(snd_buf);
    state = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, &len);
    if(state){
        perror("getsockopt() error");
        return 1;
    }

    len = sizeof(rcv_buf);
    state = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, &len);
    if(state){
        perror("getsockopt() error");
        return 1;
    }

    printf("Input buffer size: %d \n", rcv_buf);
    printf("Output buffer size: %d \n", snd_buf);
    
    close(sock);
    return 0;
}
