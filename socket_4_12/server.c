#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv){
    int sockfd, cSockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buf[1024];
    socklen_t len;

    if(argc < 2){
        printf("usage:./server localhost\n");
        return -1;
    }

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation failed");
        return -1;
    }

    // setsockopt()를 통해 SO_REUSEADDR 옵션 설정
    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //서버는 매개변수로 포트번호만 받아서 실행
    servaddr.sin_port = htons(atoi(argv[1]));

    //소켓바인드 진행
    if( bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("bind failed");
        return -1;
    }

    if( listen(sockfd, 5) < 0) {
        perror("socket failed");
        return -1;
    }

    if( (cSockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0){
        perror("accept error");
        return -1;
    }

    FILE* fp= fopen("text.txt", "r"); // "text.txt"파일을 읽기 권한으로 염 
    if(fp == NULL)
    {
        printf("파일X");
        return 0;
    }
    memset(buf, 0, sizeof(buf));
    int read_cnt;
    while(1){
        read_cnt= fread((void*)buf, 1, sizeof(buf), fp);
        if(read_cnt < sizeof(buf)){
            write(cSockfd, buf, read_cnt); // text.txt파일의 내용을 클라이언트에게 전송
            break;
        }
        write(cSockfd, buf, sizeof(buf));
    }
    // 전송하고 나면 Half close수행 -> Write Buffer를 닫고 클라이언트의 종료전에 보내는 파일데이터를 모두 받음
    shutdown(cSockfd, SHUT_WR); // Write Stream을 닫고 종료(Half close) // 소켓을 통해 read만 가능
    read(cSockfd, buf, sizeof(buf));
    printf("Message from Client\n");
    printf("%s\n", buf);
    
    // close처리
    fclose(fp);
    close(cSockfd);
    close(sockfd);
    return 0;
}