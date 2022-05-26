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
    // 매개변수로 도메인이름 하나만 입력받는 경우
    if(argc == 2){
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
        char* ip_value;
        for(int i=0; host->h_addr_list[i]; i++){
            printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
            ip_value=inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_addr.s_addr = inet_addr((const char*)ip_value); // 주소정보를 argv[2]에 받아와서 inet_addr()의 결과 리턴받음
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

    }
    // 매개변수로 포트번호와 IP 주소를 입력받는 경우
    if(argc == 3){
        int tcp_sock;
        int sock_type; // sock_type 저장 변수
        socklen_t optlen;
        int state;
        char buf[1024];
        struct sockaddr_in servaddr;

        if( (tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket creation failed");
            return -1;
        }
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(argv[2]); // ip주소를 뒤에 쓰므로 argv[2]
        servaddr.sin_port = htons(atoi(argv[1])); // port번호를 먼저 쓰므로 argv[1]
        if( connect(tcp_sock, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
            perror("connect error");
            return -1;
        }

        optlen= sizeof(sock_type);
        //getsocketopt()를 통해 소켓의 타입 정보를 받고 STREAM 타입과 같이 표준출력
        state = getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen); // socket에 적용된 type의 결과값을 4번째 인자로 넣어줌.
        if(state){
            perror("getsockopt() error");
            return 1;
        }
        printf("This socket type is : %d(%d)\n", sock_type, SOCK_STREAM);
        
        ////// 서버로부터 받은 파일 데이터를 자신의 파일로 저장하기 위해 fopen()으로 파일을 오픈
        FILE* fp= fopen("copy.txt", "w+"); //“w+” : 파일이 존재하면 해당 파일을 읽고 쓰기 둘다 가능. 파일이 존재하지 않으면 새로 생성.
        
        memset(buf, 0, sizeof(buf));
        int read_cnt;
        while( (read_cnt = read(tcp_sock, buf, sizeof(buf)))!= 0){
            fwrite((void*)buf, 1, read_cnt, fp);
        }
        //서버에게 파일 데이터를 모두 받고 문자열 출력
        puts("Received file data");
        //서버가 Half Close상태로 돌입한 후 자신이 파일에 저장했던 정보를 다시 읽어 서버에게 재전송
        while(1){
            read_cnt= fread((void*)buf, 1, sizeof(buf), fp);
            if(read_cnt < sizeof(buf)){
                write(tcp_sock, buf, read_cnt); // text.txt파일의 내용을 클라이언트에게 전송
                break;
            }
            write(tcp_sock, buf, sizeof(buf));
        }

        //close
        fclose(fp);
        close(tcp_sock);
    }
}