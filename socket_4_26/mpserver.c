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
    state= sigaction(SIGCHLD, &act, 0); // SIGCHLD: 자식이 종료되었음을 알려주는 시그널

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		error_handling("bind() error");
	}
	if (listen(serv_sock, 5)  < 0) {
		error_handling("listen() error");
	}
	
    while(1){
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if(clnt_sock== -1){
            continue;
        }else{
            puts("new client connectd...");
        }
        pid =fork();
        if(pid==-1){
            close(clnt_sock);
            continue;
        }
        if(pid==0){
            close(serv_sock);
            while((str_len=read(clnt_sock, buf, BUF_SIZE))!=0){
                write(clnt_sock, buf, str_len);
            }
            close(clnt_sock);
            puts("client disconnected...");
            return 0;
        }
        else{
            close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;
}