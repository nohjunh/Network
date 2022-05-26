#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> //도메인관련 함수 쓰려면 관련 헤더파일 선언

int main(int argc, char** argv){
    int tcp_sock, udp_sock;
    int sock_type; // sock_type 저장 변수
    socklen_t optlen;
    int state;

    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    printf("SOCK_STREAM: %d\n", SOCK_STREAM);
    printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);

    optlen= sizeof(sock_type);
    state = getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen); // socket에 적용된 type의 결과값을 4번째 인자로 넣어줌.
    if(state){
        perror("getsockopt() error");
        return 1;
    }

    printf("Socket type one: %d\n", sock_type);

    state= getsockopt(udp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen); // socket에 적용된 type의 결과값을 4번째 인자로 넣어줌.
    if(state){
        perror("getsocketopt() error");
        return 1;
    }

    printf("Socket type two: %d\n", sock_type);
    
    close(tcp_sock);
    close(udp_sock);
    return 0;
}