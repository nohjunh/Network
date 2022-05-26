#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void write_routine(int sock, char *buf){
    while(1){
        printf("Input message(Q to quit): ");
        sleep(1);
        fgets(buf, BUF_SIZE, stdin);
        if(!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")){
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}

void read_routine(int sock, char *buf){
    while(1){
        int str_len= read(sock, buf, BUF_SIZE);
        if(str_len==0){
            return;
        }
        buf[str_len]=0;
        printf("Message from server: %s", buf);
    }
}

int main(int argc, char *argv[]){
    char buf[BUF_SIZE];
    int sock;
    struct sockaddr_in serv_adr;
    pid_t pid;

    //소켓 생성
    sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr= inet_addr(argv[1]);
    serv_adr.sin_port= htons(atoi(argv[2]));

    // 서버에게 connect 요청
    if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("connect() error!");
    }else{
        puts("Connected......");
    }

    // 자식프로세스 생성
    pid= fork();
    if(pid==0){
        write_routine(sock, buf);
    } else{
        read_routine(sock, buf);
    }

    close(sock);
    return 0;
}