#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define TTL 64 // 한 홉이 지나갈수록 TTL이 1씩 줄어들고 0이 되면 패킷소멸
#define BUF_SIZE 30

void error_handling(char* message){
    fputs(message, stdout);
    fputc('\n', stdout);
    exit(1);
}

int main(int argc, char *argv[])
{   
    if(argc!=2) {
        printf("인자로 server랑 mode값 하나만 받음\n");
        exit(1);
    }

    if( strcmp(argv[1],"discovery")!=0 && strcmp(argv[1], "calc")!=0 ){
        printf("./server discovery | ./server calc\n");
        exit(1);
    }

    if( strcmp(argv[1],"discovery")==0){
        int recv_sock;
        struct sockaddr_in adr;
        int str_len;
        char buf[BUF_SIZE];

        recv_sock=socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓 생성
        memset(&adr, 0, sizeof(adr));
        adr.sin_family=AF_INET;
        adr.sin_addr.s_addr=htonl(INADDR_ANY); 
        adr.sin_port=htons(8080);
        
        if(bind(recv_sock, (struct sockaddr*)&adr, sizeof(adr))==-1)
            error_handling("bind() error");

        printf("Discovery Server operating...\n");
        char number[BUF_SIZE]="";
        bool calc_check=false;
        while(1)
        {
            str_len=recvfrom(recv_sock, buf, BUF_SIZE-1, 0, NULL, 0); // broadcast ip로부터 데이터 수신
            if(str_len < 0) 
                break;
            buf[str_len] = 0;
            char *ptr;
            if( (ptr = strstr(buf, "server:")) != NULL ){// "server: "로 저장된 문자열 찾음
                if(calc_check){
                    memset(buf, 0, sizeof(buf));
                    strcat(buf, "fail");
                }else{
                    strncpy(number, &ptr[7], 5);
                    printf("Calc Server(%s) registered\n", number);
                    memset(buf, 0, sizeof(buf));
                    strcat(buf, "success");
                    calc_check=true;
                }
                usleep(1000);
                struct sockaddr_in broad_adr;
                memset(&broad_adr, 0, sizeof(broad_adr));
                broad_adr.sin_family=AF_INET;
                broad_adr.sin_addr.s_addr=inet_addr("255.255.255.255"); // broadcast ip
                broad_adr.sin_port=htons(8081); // broadcast port 8081
                int so_brd=1; // SO_BROADCAST의 옵션을 1로 줌 ->broadcast기반 전송 가능케 함
                setsockopt(recv_sock, SOL_SOCKET, 
                 SO_BROADCAST, (void*)&so_brd, sizeof(so_brd)); //SOL_SOCKET의 옵션을 1로 지정 -> broadcast 가능
                                                       // BROADCAST 옵션 지정
                sendto(recv_sock, buf, strlen(buf), 
                0, (struct sockaddr*)&broad_adr, sizeof(broad_adr));

            }else if(strcmp(buf, "client")==0){
                memset(buf, 0, sizeof(buf));
                if(calc_check){ // calc서버가 등록되어 있었다면, port번호 전송
                    strcat(buf, number);
                }else{ // calc서버가 등록되지 않았다면, 문자열 fail 전송
                    strcat(buf, "fail");
                }
                usleep(1000);
                struct sockaddr_in broad_adr;
                memset(&broad_adr, 0, sizeof(broad_adr));
                broad_adr.sin_family=AF_INET;
                broad_adr.sin_addr.s_addr=inet_addr("255.255.255.255"); // broadcast ip
                broad_adr.sin_port=htons(8082); // broadcast port 8082
                int so_brd=1; // SO_BROADCAST의 옵션을 1로 줌 ->broadcast기반 전송 가능케 함
                setsockopt(recv_sock, SOL_SOCKET, 
                 SO_BROADCAST, (void*)&so_brd, sizeof(so_brd)); //SOL_SOCKET의 옵션을 1로 지정 -> broadcast 가능
                                                       // BROADCAST 옵션 지정
                sendto(recv_sock, buf, strlen(buf), 
                0, (struct sockaddr*)&broad_adr, sizeof(broad_adr));
            }
        }
        close(recv_sock);
    }else if( strcmp(argv[1],"calc") == 0){
        int calc_sock;
        struct sockaddr_in broad_adr;
        char buf[BUF_SIZE];
        int so_brd=1; // SO_BROADCAST의 옵션을 1로 줌 ->broadcast기반 전송 가능케 함

        calc_sock=socket(PF_INET, SOCK_DGRAM, 0); //UDP 소켓 생성
        memset(&broad_adr, 0, sizeof(broad_adr));
        broad_adr.sin_family=AF_INET;
        broad_adr.sin_addr.s_addr=inet_addr("255.255.255.255"); // broadcast ip
        broad_adr.sin_port=htons(8080); // broadcast port

        setsockopt(calc_sock, SOL_SOCKET, 
        SO_BROADCAST, (void*)&so_brd, sizeof(so_brd)); //SOL_SOCKET의 옵션을 1로 지정 -> broadcast 가능
                                                       // BROADCAST 옵션 지정

        srand(time(NULL)); // 난수 초기화
        int random = rand() % 50000 + 10000;

        printf("Register calc server\n");

        strcat(buf, "server:");
        char temp2[BUF_SIZE];
        memset(temp2, 0, sizeof(temp2));
        sprintf(temp2, "%d", random); // int값 char배열에 저장
        strcat(buf, temp2);
        // Discovery 서버가 수신대기 하고 있는 8080포트로 브로드캐스트 메세지를 문자열"server:~~~~~"로 전송함.
        sendto(calc_sock, buf, strlen(buf), 
        0, (struct sockaddr*)&broad_adr, sizeof(broad_adr));

        ////////////////////////////

        memset(buf, 0, sizeof(buf));
        struct sockaddr_in adr;
        calc_sock=socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓 생성
        memset(&adr, 0, sizeof(adr));
        adr.sin_family=AF_INET;
        adr.sin_addr.s_addr=htonl(INADDR_ANY); 
        adr.sin_port=htons(8081);

        if(bind(calc_sock, (struct sockaddr*)&adr, sizeof(adr))==-1)
            error_handling("bind() error");

        int str_len;
        str_len=recvfrom(calc_sock, buf, BUF_SIZE-1, 0, NULL, 0); // broadcast ip로부터 데이터 수신
        buf[str_len] = 0;
        if( strcmp(buf, "success") == 0){
            printf("Calc Server(%d) operating...\n", random);
        }else if(strcmp(buf, "fail") == 0){ // Fail을 받았을 경우 프로그램 종료
            printf("Fail\n");
            exit(1);
        }
        close(calc_sock);
        /////////////////////////////////////////

        int serv_sock, clnt_sock;
        struct sockaddr_in serv_adr, clnt_adr;
        struct timeval timeout;
        fd_set reads, cpy_reads;

        socklen_t adr_sz;
        int fd_max, fd_num, i;
        //char buf[BUF_SIZE];

        serv_sock= socket(PF_INET, SOCK_STREAM, 0);

        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family=AF_INET;
        serv_adr.sin_addr.s_addr=inet_addr("127.0.0.1");
        serv_adr.sin_port=htons(random); // 해당 포트번호로 TCP 소켓 생성
        
        if( bind (serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))== -1)
            error_handling("bind() error");
        if(listen(serv_sock, 5)== -1)
            error_handling("listen() error");

        
        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads); // 서버 소켓의 파일 디스크립터를 fd_set형 변수 reads에 등록
                                // 서버(listen) 소켓의 이벤트발생을 감지하겠다는 의미.
        fd_max=serv_sock; // serv_sock의 파일디스크립터 번호를 fd_max에 저장

        while(1)
        {
            cpy_reads=reads; // reads정보를 fd_set형 변수인 cpy_reads에다가 저장
            timeout.tv_sec=5;
            timeout.tv_usec=5000;
            // fd_num은 int형 변수
            // fd의 값은 0부터 시작, 하나씩 생길 때마다 1씩 증가 => 가장 큰 fd + 1를 첫 번째 인자로 전달한다.
            // maxfd는 검사 대상이 되는 파일 디스크립터 수 이므로 maxfd을 넣는 인자자리에 fd_max+1을 해서 지정된 범위까지만 관찰하도록 함.
            if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))== -1) // 서버소켓의 파일 디스크립터 번호 +1 까지의 범위 관찰
                //break;
            
            if(fd_num==0)
                continue;
            //fd_num의 값이 양수이면 이 값은 변화가 발생한 파일 디스크립터의 수를 의미
            for(i=0; i<fd_max+1; i++) // 0~fd_max+1의 디스크립터를 다 돌면서 FD_ISSET으로 이벤트 발생을 확인
            {
                // fd_set형 변수 cpy_reads에 등록된 파일디스크립터 중에서 
                // i번 파일디스크립터로 전달된 정보가 있으면 양수를 반환 (fd_max까지 다 돌거임)
                if(FD_ISSET(i, &cpy_reads)) 
                {
                    if(i==serv_sock) // connection request! 즉, 서버(listen)소켓에서 이벤트가 발생했다면,
                    {
                        adr_sz=sizeof(clnt_adr);
                        clnt_sock=
                            accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz); // 클라이언트와 연결
                        FD_SET(clnt_sock, &reads); // select를 위해 클라이언트 소켓의 파일 디스크립터 또한 reads에 등록
                                                // 클라이언트 소켓 또한 이벤트 발생을 관찰대상이 됨.
                        if(fd_max<clnt_sock) // 클라이언트 소켓까지 관찰할 수 있도록 설정했으니, 파일 디스크립터는
                         fd_max=clnt_sock; // 오름차순으로 생성됨을 고려해, 새로 만들어진 소켓 파일디스크립터까지
                                            // loop 돌기 위한 작업
                        printf("connected client: %d \n", clnt_sock); // 연결된 클라이언소켓의 파일 디스크립터 번호 출력
                    }
                    else // read message!  // i의 값이 클라이언트 소켓 파일디스크립터 번호라면,
                    {
                        char opCount, operator; // opCount를 char형태로 저장
                        int operand[1024];
                        memset(buf, 0, sizeof(buf));

                        str_len=read(i, &buf, sizeof(buf)); // 보내온 data를 읽고,
                        if(str_len==0) // close request! // 읽을 데이터가 없다면,
                        {
                            FD_CLR(i, &reads); // fd_set형 변수 reads에 등록된 i번 파일 디스크립터 정보를 삭제
                                               // 즉, 클라이언트 소켓 등록 해제
                            close(i); // 클라이언트 소켓 닫음.
                            printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
                        }
                        else // 읽을 데이터가 있다면  // 즉, str_len값이 0이 아니라면,
                        {
                            if((opCount=buf[0]) <= 0){//클라이언트가 1Byte로 전송한 operand count 정보가 char기준 0보다 작거나 같은 숫자가 나온다면 소켓 닫고 프로그램 종료
                                break;
                            }

                            for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
                                operand[i]=(int)buf[(i*4)+1]; // 각 operand는 4byte단위로 buf에 있으므로 operand count=1을 제외하고부터 4바이트씩 읽음.
                            }
                            int result = operand[0]; // 시작값 index 0 부터
                            for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
                                operator=buf[(opCount*4)+1 +i]; // operator는 피연산자수+피연산자들 그 다음부터 buf에 있으므로 다음처럼 식을 잡음.
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
                            write(i, &result, 4); // 계산된 결과를 클라이언트에게 전송
                            FD_CLR(i, &reads); // fd_set형 변수 reads에 등록된 i번 파일 디스크립터 정보를 삭제
                                               // 즉, 클라이언트 소켓 등록 해제
                            close(i);
                            printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
                        }
                    }
                }
            }
        }
        close(serv_sock); // 서버소켓 닫음.
    }
    return 0;
}