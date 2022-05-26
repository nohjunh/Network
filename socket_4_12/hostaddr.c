#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> //도메인관련 함수 쓰려면 관련 헤더파일 선언

int main(int argc, char** argv){
    struct hostent *host, *host2;
    struct sockaddr_in addr;

    if(argc < 2){
        printf("Usage: %s <DomainName> <RemoteAddress>\n", argv[0]);
        return 1;
    }

    host= gethostbyname(argv[1]); // argv[1]로 dns요청하고 결과를 hostent 구조체에 저장(구조체 변수의 주소값이 리턴됨.)
    if(!host){
        perror("gethostbyname() error");
        return 1;
    }

    printf("gethostbyname()\n");
    printf("Official name: %s \n", host->h_name);
    for(int i=0; host->h_aliases[i]; i++){
           printf("Aliases %d: %s\n", i, host->h_aliases[i]);
    }
    printf("Address type: %s\n", (host->h_addrtype==AF_INET)? "AF_INET": "AF_INF6");
    for(int i=0; host->h_addr_list[i]; i++){
        printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }



    if(argc == 2){
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(argv[2]); // 주소정보를 argv[2]에 받아와서 inet_addr()의 결과 리턴받음
    host= gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET); // inet_addr(argv[2])로 DNS 요청을 하고 결과를 hostent 구조체에 저장
    if(!host){
        perror("gethostbyaddr() error");
        return 1;
    }

    printf("\ngethostbyaddr()\n");
    printf("Offcial name: %s \n", host->h_name);
    for(int i=0; host->h_aliases[i]; i++){
        printf("Aliases %d: %s \n", i, host->h_aliases[i]);
    }

    printf("Address type: %s\n", (host->h_addrtype==AF_INET)? "AF_INET" : "AF_INET6");
    for(int i=0; host->h_addr_list[i]; i++){
        printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr= inet_addr(argv[2]);
    host = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);

    if(!host){
        perror("gethostbyaddr() error");
        return 1;
    }

}