#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TTL 64 // 한 홉이 지나갈수록 TTL이 1씩 줄어들고 0이 되면 패킷소멸
#define BUF_SIZE 30
void error_handling(char *message);


int main(int argc, char *argv[])
{
    int recv_sock;
    struct sockaddr_in adr;
    int str_len;
    char buf[BUF_SIZE];
    
    if(argc!=2) {
        printf("Usage : %s  <PORT>\n", argv[0]);
        exit(1);
    }

    recv_sock=socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓 생성
    memset(&adr, 0, sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY); 
    adr.sin_port=htons(atoi(argv[1]));
    
    if(bind(recv_sock, (struct sockaddr*)&adr, sizeof(adr))==-1)
        error_handling("bind() error");
    
    while(1)
    {
        str_len=recvfrom(recv_sock, buf, BUF_SIZE-1, 0, NULL, 0); // broadcast ip로부터 데이터 수신
        if(str_len<0) 
            break;
        buf[str_len]=0;
        fputs(buf, stdout);
    }
    
    close(recv_sock);
    return 0;
}

void error_handling(char* message){
    fputs(message, stdout);
    fputc('\n', stdout);
    exit(1);
}