#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv){
    int sockfd, cSockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buf[1024];
    socklen_t len;
    char opCount, operator; // opCount를 char형태로 저장
    int operand[1024];

    if(argc < 2){
        printf("usage:./server localhost\n");
        return -1;
    }

    if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket creation failed");
        return -1;
    }

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY와 매개변수로 받은 포트번호
    servaddr.sin_port = htons(atoi(argv[1]));

    if( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){ // 소켓 바인드 진행
        perror("bind failed");
        return -1;
    }

    while(1){ // 반복된 클라이언트 요청에도 문제없이 결과가 전송되어야 함. // 서버는 iterative 형태로 구현.

        len=sizeof(cliaddr); // socket address structure size
        recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &len); // 클라이언트가 전송한 operand count정보 및 피연산자들, 연산자들을 char형태로 저장
        if((opCount=buf[0]) <= 0){//클라이언트가 1Byte로 전송한 operand count 정보가 char기준 0보다 작거나 같은 숫자가 나온다면 반복 빠져나와서 (while문 밖에서 소켓 닫고 프로그램 종료)
            printf("Server close(%d)\n", opCount); // Server close(operand count) 출력
            break;
        }
        printf("Operand count: %d\n", opCount); // opCount는 char형으로 선언

        for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
            operand[i]=(int)buf[(i*4)+1]; // 각 operand는 4byte단위로 buf에 있으므로 operand count=1을 제외하고부터 4바이트씩 읽음.
            printf("Operand %d: %d\n", i, operand[i]);
        }
        int result = operand[0]; // 시작값 index 0 부터
        for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
            operator=buf[(opCount*4)+1 +i]; // operator는 피연산자수+피연산자들 그 다음부터 buf에 있으므로 다음처럼 식을 잡음.
            printf("Operator %d: %c\n", i, operator);
            switch(operator){
            case '+':
                result+=operand[i+1]; // 시작값이 index 0 부터 시작이므로 그 다음 operand부터 잡아야한다. 따라서 i+1
                break;
            case '-':
                result-=operand[i+1];
                break;
            case '*':
                result*=operand[i+1];
                break;
            default:
                break;
            }
        }

        printf("Operation result: %d\n", result); // 전송 전에 결과 출력
        sendto(sockfd, &result, 4, 0, (struct sockaddr*)&cliaddr, len); // 계산된 결과를 클라이언트에게 전송
    }
    close(sockfd); // 프로그램 종료조건에 의해서 while문 빠져나오면 소켓닫고 프로그램 종료.
    return 0;
}