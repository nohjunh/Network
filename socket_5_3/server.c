#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include <sys/uio.h> // writev, readv

#define BUF_SIZE 1024


void read_childproc(int sig){
    pid_t pid;
    int status;
    pid=waitpid(-1, &status, WNOHANG); // Non-Block상태로 임의의 자식 프로세스 종료대기. Blocking상태로 쭉 대기하지 않고 자식 프로세스의 종료상태 체크 가능
    // waitpid로 1. Non Blocking으로 종료 신호 대기
    //           2. 자식의 반환 값을 받아 좀비 프로세스 소멸
    printf("removed proc id: %d\n", pid);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[]){
    int fds1[2], fds2[2];

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;

    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;

    struct timeval timeout;
	fd_set reads, cpy_reads;
	int fd_max, str_len, fd_num, i;

    int state;
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    if(argc!=2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    ////////////// sigaction()을 통해 자식프로세스가 종료되면, 자식이 종료되었다는 메세지와 함께 pid 출력/////////////
    act.sa_handler= read_childproc; // sa_handler에 호출할 함수 저장
    sigemptyset(&act.sa_mask); // sa_mask랑 flags를 0으로 초기화
    act.sa_flags=0;
    state= sigaction(SIGCHLD, &act, 0); // SIGCHLD: 자식이 종료되었음을 알려주는 시그널 // zombie 처리


    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		error_handling("bind() error");
	}
	if (listen(serv_sock, 5) == -1) {
		error_handling("listen() error");
	}
	
    /////////////////파이프를 생성하고 자식프로세스 생성///////////////////////////
    // 파이프를 먼저 2개 생성
    pipe(fds1), pipe(fds2); // 파이프 2개를 써서 통신
    pid= fork(); // 자식프로세스가 pipe값 그대로 가지고 fork

    if(pid==0){ // 자식프로세스 분기문
        struct iovec vec[3];
        char buf_mode[BUF_SIZE]={0, };
        char buf_ID[BUF_SIZE]={0, };
        char buf_calcData[BUF_SIZE]={0, };
        int str_len;
        
        vec[0].iov_base= buf_mode;
        vec[0].iov_len=4;
        vec[1].iov_base= buf_ID;
        vec[1].iov_len= 4;
        vec[2].iov_base= buf_calcData;
        vec[2].iov_len = BUF_SIZE;  // 버퍼에 있는 값을 다 읽어라.

        int count=0;
        char read_array[30][30]={ 0 };
        char msgbuf[BUF_SIZE];
        memset(msgbuf, 0, sizeof(msgbuf));
        int len;
        // 파이프를 통해 들어오는 데이터를 받을 때까지 readv로 대기
        while(1){
            //부모프로세스에게서 fds1[0]으로 data를 수신
            str_len=readv(fds1[0], vec, 3);
            if(strcmp(buf_mode, "save")==0){
                strcat(read_array[count], buf_calcData);
                count++;
            }else if(strcmp(buf_mode, "load")== 0){
                memset(msgbuf, 0, sizeof(msgbuf));
                for(int i=0; i<count; i++){
                    char *ptr= strstr(read_array[i], buf_ID); // buf_ID로 저장된 문자열 찾음
                    if(ptr!=NULL){ // 해당 buf_ID로 된 문자열이 있다면, 하나의 문자열로 만듬
                        strcat(msgbuf, read_array[i]);
                        char linechange[]="\n";
                        strcat(msgbuf, linechange);
                    }
                }
                if(strlen(msgbuf)==0){ // 문자열 데이터가 없었을 경우 Not exist를 전송
                    strcat(msgbuf, "Not exist\n");        
                }
                // 부모프로세스에게 문자열 데이터 전달
                write(fds2[1], msgbuf, sizeof(msgbuf)); //  파이프 fds2([1])의 송신용도 방향으로 write
            }else if(strcmp(buf_mode, "quit")== 0){
                //자식 프로세스는 quit 정보를 받으면 자신을 종료함
                break;
            }else{
                continue;
            }
        }
        printf("quit\n");
        return 0;
    }
    else // 부모프로세스 분기문
    {
        struct iovec vec[3];
        char buf_mode[BUF_SIZE]={0, };
        char buf_ID[BUF_SIZE]={0, };
        char buf_calcData[BUF_SIZE]={0, };
        int str_len;
        
        vec[0].iov_base= buf_mode;
        vec[0].iov_len=4;
        vec[1].iov_base= buf_ID;
        vec[1].iov_len= 4;
        vec[2].iov_base= buf_calcData;
        vec[2].iov_len = BUF_SIZE;  // 버퍼에 있는 값을 다 읽어라.

        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads); // 서버 소켓의 파일 디스크립터를 fd_set형 변수 reads에 등록
                                // 서버(listen) 소켓의 이벤트발생을 감지하겠다는 의미.
        fd_max=serv_sock; // serv_sock의 파일디스크립터 번호를 fd_max에 저장

        while(1)
        {
            if(strcmp(buf_mode, "quit")==0){
                break;
            }
            cpy_reads=reads; // reads정보를 fd_set형 변수인 cpy_reads에다가 저장
            timeout.tv_sec=5;
            timeout.tv_usec=5000;
            // fd_num은 int형 변수
            // fd의 값은 0부터 시작, 하나씩 생길 때마다 1씩 증가 => 가장 큰 fd + 1를 첫 번째 인자로 전달한다.
            // maxfd는 검사 대상이 되는 파일 디스크립터 수 이므로 maxfd을 넣는 인자자리에 fd_max+1을 해서 지정된 범위까지만 관찰하도록 함.
            if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))== -1) // 서버소켓의 파일 디스크립터 번호 +1 까지의 범위 관찰
                break;
            
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
                        str_len=readv(i, vec, 3); // 보내온 data를 읽고,
                        if(str_len==0) // close request! // 읽을 데이터가 없다면,
                        {
                            FD_CLR(i, &reads); // fd_set형 변수 reads에 등록된 i번 파일 디스크립터 정보를 삭제
                                               // 즉, 클라이언트 소켓 등록 해제
                            close(i); // 클라이언트 소켓 닫음.
                            printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
                        }
                        else // 읽을 데이터가 있다면  // 즉, str_len값이 0이 아니라면,
                        {
                            char opCount, operator; // opCount를 char형태로 저장
                            int operand[1024];
                            memset(operand, 0, sizeof(operand));
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
                                opCount= buf_calcData[1]-48;
                                if(opCount <= 0){ // operand count 정보가 char기준 0보다 작같 숫자가 나온다면 반복 빠져나감.
                                    FD_CLR(i, &reads); // fd_set형 변수 reads에 등록된 i번 파일 디스크립터 정보를 삭제
                                                // 즉, 클라이언트 소켓 등록 해제
                                    close(i); // 클라이언트 소켓 닫음.
                                    printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
                                    close(clnt_sock);
                                    break;
                                }
                                printf("save to %s \n", buf_ID);
                                char temp2[BUF_SIZE];
                                memset(temp2, 0, sizeof(temp2));
                                strcat(buf, buf_ID);
                                strcat(buf, ": ");

                                for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
                                    operand[i]= buf_calcData[1+i+1]-48; // char형을 int형으로
                                }
                                int result = operand[0]; // 시작값 index 0 부터
                                sprintf(temp2, "%d", operand[0]); // int값 char배열에 저장
                                strcat(buf, temp2);
                                for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
                                    operator= buf_calcData[1+opCount+1+i];
                                    switch(operator){
                                    case '+':
                                        result+=operand[i+1]; // 시작값이 index 0 부터 시작이므로 그 다음 operand부터 잡아야한다. 따라서 i+1
                                        strcat(buf, "+");
                                        sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                                        strcat(buf, temp2);
                                        break;
                                    case '-':
                                        result-=operand[i+1];
                                        strcat(buf, "-");
                                        sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                                        strcat(buf, temp2);
                                        break;
                                    case '*':
                                        result*=operand[i+1];
                                        strcat(buf, "*");
                                        sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                                        strcat(buf, temp2);
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                write(clnt_sock, &result, 4); // 계산된 결과를 클라이언트에게 전송
                                strcat(buf, "=");
                                sprintf(temp2, "%d", result); // int값 char배열에 저장
                                strcat(buf, temp2);
                                strcpy(buf_calcData, buf);
                                writev(fds1[1], vec, 3);  // pipe를 통해 이전 자식프로세스에게도 계산결과를 보냄
                                memset(buf, 0, sizeof(buf));
                                break;
                            case 2:
                                printf("load from %s \n", buf_ID);
                                writev(fds1[1], vec, 3);  // pipe를 통해 mode와 id정보 보냄
                                read(fds2[0], buf, sizeof(buf)); // 파이프 fds2([0])의 수신용도 방향으로 read
                                write(clnt_sock, buf, sizeof(buf)); // 자식프로세스에게 받은 결과를 클라이언트에게 전송
                                break;
                            case 3:
                                writev(fds1[1], vec, 3);  // pipe를 통해 자식프로세스에게 정보를 넘김
                                sleep(1);
                                close(i); // 클라이언트 소켓 닫음.
                                printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
                                break;
                            }
                        }
                    }
                }
            }
        }
        close(serv_sock);
        return 0;
    }
}