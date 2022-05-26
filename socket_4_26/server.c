#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#define BUF_SIZE 1024

void read_childproc(int sig){
    pid_t pid;
    int status;
    pid=waitpid(-1, &status, WNOHANG); // Non-Block상태로 임의의 자식 프로세스 종료대기. Blocking상태로 쭉 대기하지 않고 자식 프로세스의 종료상태 체크 가능
    // waitpid로 1. Non Blocking으로 종료 신호 대기
    //           2. 자식의 반환 값을 받아 좀비 프로세스 소멸
    printf("Removed proc id: %d\n", pid);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[]){
    int fds[2];

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;

    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;

    int str_len, state;
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
    pipe(fds); // 파이프를 먼저 생성
    pid=fork(); // 자식프로세스가 pipe값 그대로 가지고 fork

    if(pid==0){ // 자식프로세스 분기문
        FILE * fp= fopen("log.txt", "wt"); // 자식프로세스는 "log.txt" 파일 생성 및 오픈
        char msgbuf[BUF_SIZE];
        memset(msgbuf, 0, sizeof(msgbuf));
        int i, len;
        // 파이프를 통해 들어오는 데이터를 받을 때까지 read로 대기
        while(1){
            //부모프로세스에게서 fds[0]으로 data를 수신해 파일에 write
            len= read(fds[0], msgbuf, BUF_SIZE); // read에 의해 block됨.
            if(strcmp(msgbuf, "end") == 0){ // 만약 msgbuf의 값이 "end"라면 파일을 담당하던 자식 프로세스를 닫으라는 소리이므로 break로 while문 벗어남.
                break;
            }
            printf("%s\n", msgbuf);
            strcat(msgbuf, "\n");
            fwrite((void*)msgbuf, 1, strlen(msgbuf), fp);
            msgbuf[strlen(msgbuf) - 1] = '\0';
        }
        fclose(fp);
        return 0;
    }
    else // 부모프로세스 분기문
    { 
     while(1){
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if(clnt_sock== -1){
            continue;
        }else{
            puts("new client connected..."); // 클라이언트의 요청을 받으면 해당 문자열 출력
        }
        pid =fork(); // 부모프로세스에서 자식프로세스 생성
        if(pid==-1){
            close(clnt_sock);
            continue;
        }
        if(pid==0){ /////////// 자식프로세스 분기문 -> 클라이언트에게 서비스를 제공할 수 있도록 구현///////////////////
            close(serv_sock); // 클라이언트에게 data를 전송할 것이므로 클라이언트 소켓만 있으면 된다.
                              // 따라서, 서버소켓은 그냥 닫음.

            char opCount, operator; // opCount를 char형태로 저장
            int operand[1024];
            read(clnt_sock, &opCount, 1); // 클라이언트가 1Byte로 전송한 operand count 정보를 char형태로 저장
            if(opCount <= 0){ // operand count 정보가 char기준 0보다 작거나 같은 숫자가 나온다면 반복 빠져나감.
                printf("Savefile(%d)\n", opCount); // Server close(operand count) 출력
                int temp= (int)getpid();
                char temp2[BUF_SIZE];
                memset(temp2, 0, sizeof(temp2));
                strcat(temp2, "end");
                write(fds[1], temp2, sizeof(temp2)); // 자식프로세스에게 end를 보내서 종료를 알림.
                close(clnt_sock);
                //break;
            }
            
            int temp= (int)getpid();
            char temp2[BUF_SIZE];
            memset(temp2, 0, sizeof(temp2));
            sprintf(temp2, "%d", temp); // int값 char배열에 저장
            strcat(buf, temp2);
            strcat(buf, ": ");
            for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
                read(clnt_sock, &operand[i], 4);
            }
            int result = operand[0]; // 시작값 index 0 부터
            sprintf(temp2, "%d", operand[0]); // int값 char배열에 저장
            strcat(buf, temp2);
            for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
                read(clnt_sock, &operator, 1);
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
            write(fds[1], buf, strlen(buf));  // pipe를 통해 이전 자식프로세스에게도 계산결과를 보냄
            close(clnt_sock);
            return 0;
        }
        else{ // 부모프로세스 분기문
            close(clnt_sock); // 부모는 클라이언트소켓을 닫고 다시 while문으로 돌아가
                              // 다음 클라이언트를 accept할 준비를 함.
        }
    }
    close(serv_sock);
    return 0;
    }
}
