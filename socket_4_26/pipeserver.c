#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

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
    int fds[2];

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;

    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;

    int str_len, state;
    char buf[BUF_SIZE];
    if(argc!=2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    act.sa_handler= read_childproc; // sa_handler에 호출할 함수 저장
    sigemptyset(&act.sa_mask); // sa_mask랑 flags를 0으로 초기화
    act.sa_flags=0;
    state= sigaction(SIGCHLD, &act, 0); // SIGCHLD: 자식이 종료되었음을 알려주는 시그널 // zombie 처리

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
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
	
    pipe(fds); // 파이프 생성
    pid=fork(); // 자식프로세스가 pipe값 그대로 가지고 fork
    
    if(pid==0){ // 자식프로세스 분기문
        FILE * fp= fopen("echomsg.txt", "wt");
        char msgbuf[BUF_SIZE];
        int i, len;

        for(i=0; i<3; i++){
            //부모프로세스에게서 fds[0]으로 data를 수신해 파일에 write
            len= read(fds[0], msgbuf, BUF_SIZE); // read에 의해 block됨. 
            fwrite((void*)msgbuf, 1, len, fp);
        }
        fclose(fp);
        return 0;  // 10번 data를 수신받으면 이 자식프로세스는 종료될 것이다.
    }
    else // 부모프로세스 분기문
    { 
     while(1){
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if(clnt_sock== -1){
            continue;
        }else{
            puts("new client connectd...");
        }

        pid =fork(); // 부모프로세스에서 자식프로세스 생성
        if(pid==-1){
            close(clnt_sock);
            continue;
        }
        if(pid==0){ // 자식프로세스 분기문
            close(serv_sock); // 클라이언트에게 data를 전송할 것이므로 클라이언트 소켓만 있으면 된다.
                              // 따라서, 서버소켓은 그냥 닫음.
            while((str_len=read(clnt_sock, buf, BUF_SIZE)) != 0){ // 클라이언트로부터 data를 받음
                write(clnt_sock, buf, str_len); // 연결된 클라이언트에게 data 보냄
                write(fds[1], buf, str_len); // pipe를 통해 이전 자식프로세스에게 data 보냄
            }
            close(clnt_sock);
            puts("client disconnected...");
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
