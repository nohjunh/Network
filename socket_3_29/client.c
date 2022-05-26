#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    int sockfd;
    char buf[1024];
    struct sockaddr_in servaddr;
    int opCount;
    int opResult;

    if(argc < 3){
        printf("usage:./client remoteAddress remotePort\n");
        return -1;
    }

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation failed");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[2]); // ip주소를 뒤에 쓰므로 argv[2]
    servaddr.sin_port = htons(atoi(argv[1])); // port번호를 먼저 쓰므로 argv[1]

    if( connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("connect error");
        return -1;
    }

    fputs("Operand count: ", stdout);
    scanf("%d", &opCount);
    buf[0] = (char)opCount; // opCount가 4바이트 정수이므로 1바이트인 char로 타입캐스팅 -> 128부터 overflow
                            // unsigned char라면 음수없이 255까지 표현가능
                            // char는 128~255를 음수로 표현하며 이를 정상동작으로 취급. unsigned char는 저 범위의 값을 양수로 하여 출력하면 오작동으로 취급. 
    
    if(buf[0] <=0 ){ // operand 전송 시 char 기준 0보다 작거나 같은 값을 전송하는 케이스면 서버에게 전송하고 추가적인 표준입력 없이 소켓닫고 종료
        write(sockfd, buf, 1);
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