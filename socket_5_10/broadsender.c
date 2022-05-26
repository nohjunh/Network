#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int send_sock;
    struct sockaddr_in broad_adr;
    FILE *fp;
    char buf[BUF_SIZE];
    int so_brd=1; // SO_BROADCAST의 옵션을 1로 줌 -> broadcast 기반 전송 가능케 함
    
    if(argc!=3) {
        printf("Usage : %s <Boradcast IP> <PORT>\n", argv[0]);
        exit(1);
    }

    send_sock=socket(PF_INET, SOCK_DGRAM, 0); //UDP 소켓 생성
    memset(&broad_adr, 0, sizeof(broad_adr));
    broad_adr.sin_family=AF_INET;
    broad_adr.sin_addr.s_addr=inet_addr(argv[1]); // broadcast ip
    broad_adr.sin_port=htons(atoi(argv[2])); // broadcast port

    setsockopt(send_sock, SOL_SOCKET, 
    SO_BROADCAST, (void*)&so_brd, sizeof(so_brd)); //SOL_SOCKET의 옵션을 1로 지정 -> broadcast 가능
                                                   // BROADCAST 옵션 지정

    if((fp=fopen("news.txt", "r")) == NULL)
     error_handling("fopen() error");

    while(!feof(fp))
    {
        fgets(buf, BUF_SIZE, fp);
        sendto(send_sock, buf, strlen(buf), 
        0, (struct sockaddr*)&broad_adr, sizeof(broad_adr));
        sleep(2);
    }

    close(send_sock);
    return 0;
}

void error_handling(char* message){
    fputs(message, stdout);
    fputc('\n', stdout);
    exit(1);
}