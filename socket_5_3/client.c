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

char buf_mode[BUF_SIZE];
char buf_ID[BUF_SIZE];
char buf_calcData[BUF_SIZE];


void get_ID(){
    printf("ID: ");
    scanf("%s", buf_ID);;
    if(strlen(buf_ID) != 4){ // 문자열 길이가 4가 아니라면 아래 문자열 출력 후 종료
        printf("Error: ID length must be 4\n");
        exit(0);
    }
}

int main(int argc, char *argv[]){
    int sockfd;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    struct sockaddr_in servaddr;
    int opCount;
    int opResult;

    struct iovec vec[3];
    int str_len;

    /////////////////////// 서버와 연결
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

    printf("Mode: ");
    scanf("%s", buf_mode);

    int mode_int=0; // mode를 나타내는 int형 변수 1==save, 2==load, 3==quit
    if(strcmp(buf_mode, "save") == 0){
        mode_int = 1;
    }else if(strcmp(buf_mode, "load") == 0){
        mode_int = 2;
    }else if(strcmp(buf_mode, "quit") == 0){
        mode_int = 3;
    }else{
        mode_int = 4; // save, load, quit이 아닌 다른 mode값을 받았다면 4로 지정 
    }

    switch (mode_int)
    {
    case 1: // 1==save
        // Mode가 save와 load라면 ID정보를 입력받음. 이때 ID값의 길이는 항상 4로 고정
        get_ID();
        memset(buf_calcData, 0, sizeof(buf_calcData));
        // lovec를 활용해서 첫 번째 배열( vec[0] )에는 mode, 두 번째 배열( vec[1] )에는 ID, 세 번째 배열( vec[3] )에는 계산 정보를 넣음. 
        // iov_base는 버퍼의 주소 정보
        // iov_len은 버퍼의 크기정보
        vec[0].iov_base= buf_mode;
        vec[0].iov_len= 4; // save, load, quit 의 모드 3가지도 4글자이므로 len을 4로 함. 
        vec[1].iov_base= buf_ID;
        vec[1].iov_len= 4; // ID는 4글자로 고정

        fputs("Operand count: ", stdout);
        scanf("%d", &opCount);
        buf[0] = (char)opCount; // opCount가 4바이트 정수이므로 1바이트인 char로 타입캐스팅 -> 128부터 overflow
                                // unsigned char라면 음수없이 255까지 표현가능
                                // char는 128~255를 음수로 표현하며 이를 정상동작으로 취급. unsigned char는 저 범위의 값을 양수로 하여 출력하면 오작동으로 취급.
        char temp2[BUF_SIZE];
        memset(temp2, 0, sizeof(temp2));
        sprintf(temp2, "%d", opCount); // int값 char배열에 저장
        strcat(buf, temp2);

        if(opCount <= 0 ){ // operand 전송 시 char 기준 0보다 같작 값을 받는 케이스면 표준 출력하고 소켓 닫고 종료
            printf("Overflow will happen(%d)\n", opCount);
            close(sockfd);
            return 0;
        }

        int operand;
        for(int i=0; i<opCount; i++){
            printf("Operand %d: ", i); // operand수 만큼 입력받음
            scanf("%d", &operand); 
            sprintf(temp2, "%d", operand); // int값 char배열에 저장
            strcat(buf, temp2);
        }
        
        char operator;
        for(int i=0; i<opCount-1; i++){ // (operand count)-1의 수 만큼 operator 입력받음
            printf("Operator %d: ", i);
            scanf(" %c", &operator); // 마지막 버퍼에 연산자 받음
            temp2[0]=operator;
            strcat(buf, temp2);
        }

        strcpy(buf_calcData, buf);
        vec[2].iov_base= buf_calcData; 
        vec[2].iov_len= strlen(buf_calcData);
        // writev함수
        // 첫번째 인자는 데이터 전송의 목적지를 나타내는 소켓의 파일디스크립터
        // 두번째 인자는 구조체 iovec 배열의 주소 값 전달
        // 세번째 인자는 두 번째 인자로 전달된 주소 값이 가리키는 배열의 길이정보 전달
        writev(sockfd, vec, 3); // 한번에 writev()로 전송

        // 서버에게 계산 결과를 받게 되면 이를 표준출력으로 출력하고 소켓을 닫고 종료
        read(sockfd, &opResult, 4);
        printf("Operation result: %d\n", opResult); // 표준출력으로 Operation result: 와 함께 출력
        close(sockfd);
        break;
    case 2: // 2==load
        // Mode가 save와 load라면 ID정보를 입력받음. 이때 ID값의 길이는 항상 4로 고정
        get_ID();
        char read_array[BUF_SIZE];
        memset(read_array, 0 ,sizeof(read_array));

        vec[0].iov_base= buf_mode;
        vec[0].iov_len= 4; // save, load, quit 의 모드 3가지도 4글자이므로 len을 4로 함. 
        vec[1].iov_base= buf_ID; 
        vec[1].iov_len= 4; // ID는 4글자로 고정
        // 세 번째 인자의 경우 사용하지 않으므로 특정한 값을 넣지 않고 writev로 전달
        vec[2].iov_base= buf_calcData;
        vec[2].iov_len= 0;
        writev(sockfd, vec, 3);
        // 서버가 ID에 해당하는 계산 결과 정보들을 문자열로 보내주면 이를 read()로 받아 출력, 소켓 닫고 종료
        int check= read(sockfd, &read_array, sizeof(read_array));
        if(check==0){ // 해당하는 ID에 대해 값을 제대로 받지 못하면
            printf("Not exist");
            close(sockfd);
            return 0;
        }else{
            read_array[strlen(read_array) - 1] = '\0'; // 문자열 맨 마지막 줄바꿈 삭제.
            printf("%s\n", read_array);
        }
        close(sockfd);
        break;
    case 3: // 3==quit
        vec[0].iov_base= buf_mode;
        vec[0].iov_len= 4; // save, load, quit 의 모드 3가지도 4글자이므로 len을 4로 함.
        // 두 번째 인자의 경우 사용하지 않으므로 특정한 값을 넣지 않고 writev로 전달
        vec[1].iov_base= buf_ID;
        vec[1].iov_len= 0; 
        // 세 번째 인자의 경우 사용하지 않으므로 특정한 값을 넣지 않고 writev로 전달
        vec[2].iov_base= buf_calcData;
        vec[2].iov_len= 0;

        writev(sockfd, vec, 3);
        // 소켓 닫고 종료
        close(sockfd);
        break;
    default: // save, load, quit 이외의 값이 들어오면 문자열 출력 후 종료
        printf("supported mode: save load quit\n");
        break;
    }
    return 0;
}
