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

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

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

    memset(buf, 0, sizeof(buf));
    read(cSockfd, buf, sizeof(buf));
    printf("%s\n", buf);

    strcat(buf, "_노준혁");
    write(cSockfd, buf, strlen(buf));

    close(cSockfd);
    close(sockfd);
    return 0;
}