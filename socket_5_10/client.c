#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUF_SIZE 1024

void error_handling(char* message){
    fputs(message, stdout);
    fputc('\n', stdout);
    exit(1);
}

char buf[BUF_SIZE];

int main(int argc, char** argv) {
    printf("Start to find calc server\n");
    
    //8080번 포트로 수신 대기하고 있는 discovery서버에게 브로드캐스트 메시지로 문자열 "client" 전송
    int client_sock;
    struct sockaddr_in broad_adr;
    memset(buf, 0, sizeof(buf));
    int so_brd=1; // SO_BROADCAST의 옵션을 1로 줌 ->broadcast기반 전송 가능케 함

    client_sock=socket(PF_INET, SOCK_DGRAM, 0); //UDP 소켓 생성
    memset(&broad_adr, 0, sizeof(broad_adr));
    broad_adr.sin_family=AF_INET;
    broad_adr.sin_addr.s_addr=inet_addr("255.255.255.255"); // broadcast ip
    broad_adr.sin_port=htons(8080); // broadcast port

    setsockopt(client_sock, SOL_SOCKET, 
    SO_BROADCAST, (void*)&so_brd, sizeof(so_brd)); //SOL_SOCKET의 옵션을 1로 지정 -> broadcast 가능
                                                       // BROADCAST 옵션 지정

    strcat(buf, "client");
    // Discovery 서버가 수신대기 하고 있는 8080포트로 브로드캐스트 메세지를 문자열"client"로 전송함.
    sendto(client_sock, buf, strlen(buf), 
    0, (struct sockaddr*)&broad_adr, sizeof(broad_adr));
    //////////////////////////////////////////////////////

    memset(buf, 0, sizeof(buf));
    struct sockaddr_in adr;
    client_sock=socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓 생성
    memset(&adr, 0, sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY); 
    adr.sin_port=htons(8082); // 8082번 포트로 소켓을 생성하고 discovery서버의 메세지를 수신 대기함.

    if(bind(client_sock, (struct sockaddr*)&adr, sizeof(adr))==-1)
        error_handling("bind() error");

    int str_len;
    str_len=recvfrom(client_sock, buf, BUF_SIZE-1, 0, NULL, 0); // broadcast ip로부터 데이터 수신
    buf[str_len] = 0;
    if( strcmp(buf, "fail") == 0){ // Fail을 받았을 경우 프로그램 종료
        printf("Fail\n");
        exit(1);
    }else{ //  포트번호를 받았으면,
        printf("Found calc server(%s)\n", buf);
    }
    close(client_sock);
///////////////////////////////////////
    int sockfd;
    struct sockaddr_in servaddr;
    int opCount;
    int opResult;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation failed");
        return -1;
    }
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(atoi(buf)); // Calc서버의 포트번호를 받았으므로 이 port번호로 설정


    if( connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("connect error");
        return -1;
    }

    memset(buf, 0, sizeof(buf));

    fputs("Operand count: ", stdout);
    scanf("%d", &opCount);
    buf[0] = (char)opCount; // opCount가 4바이트 정수이므로 1바이트인 char로 타입캐스팅 -> 128부터 overflow
                            // unsigned char라면 음수없이 255까지 표현가능
                            // char는 128~255를 음수로 표현하며 이를 정상동작으로 취급. unsigned char는 저 범위의 값을 양수로 하여 출력하면 오작동으로 취급. 
    
    if(buf[0] <=0 ){ // operand 전송 시 char 기준 0보다 작거나 같은 값을 전송하는 케이스면 추가적인 표준입력 없이 소켓닫고 종료
        close(sockfd);
        return 0;
    }

    for(int i=0; i<opCount; i++){
        printf("Operand %d: ", i); // operand수 만큼 입력받음
        scanf("%d", (int*)&buf[(i*4)+1]); // 1부터 시작해야 4Byte씩 끊어짐.
    }
    
    for(int i=0; i<opCount-1; i++){ // (operand count)-1의 수 만큼 operator 입력받음
        printf("Operator %d: ", i);
        scanf(" %c", &buf[(opCount*4)+i+1]); // 마지막 버퍼에 연산자 받음
    }
    write(sockfd, buf, (opCount*4)+1+(opCount-1)); // 피연산자수Byte + 피연산자들 Byte + 연산자들 Byte // 표준입력으로 받은 데이터를 char배열로
                                                   // 받아 한번에 write()로 전송

    read(sockfd, &opResult, 4);
    printf("Operation result: %d\n", opResult); // 표준출력으로 Operation result: 와 함께 출력하고 소켓을 닫고 종료

    close(sockfd);
    return 0;
}