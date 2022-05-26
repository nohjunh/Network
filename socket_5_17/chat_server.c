/*
쓰레드를 소멸하는 2가지 방법
1. pthread_join()으로 쓰레드를 소멸시킬 수 있지만,
   이 함수는 쓰레드가 종료 될 때까지 블로킹 상태에 놓이게 됨.
2. pthread_detach() : pthread를 프로세스와 분리시킴.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

// 공유변수로 사용됨. 즉, 임계영역을 구성 -> 현재 연결된 클라이언트 수를 세는 용도의 변수들
int clnt_cnt=0;  // 서버에 접속한 클라이언트의 소켓들을 관리하는 변수
int clnt_socks[MAX_CLNT]; 
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;

    pthread_t t_id;

    if(argc!=2) {
      printf("Usage : %s <port>\n", argv[0]);
      exit(1);
    }

    pthread_mutex_init(&mutx, NULL); // 뮤텍스 초기화
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET; 
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));
   
    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
       error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
       error_handling("listen() error");

     while(1)
     {
       clnt_adr_sz=sizeof(clnt_adr);
       clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
       
       // 클라이언트 소켓 fd 추가를 위한 Mutex
       pthread_mutex_lock(&mutx);
       // 새로운 연결 형성 시 해당 정보 등록하는 과정
       clnt_socks[clnt_cnt++]=clnt_sock; // clnt_socks와 clnt_cnt는 공유변수이므로 처리 과정중에
                                         // mutex를 통해 다른 스레드가 접근하지 못하도록 함.
                                         // 파일 디스크립터 값이 배열에 저장될 것이다.
       pthread_mutex_unlock(&mutx); // unlock
   
       pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); // 연결된 클라이언트 파일 디스크립터 번호를 스타트루틴의 인자로 전달
       //detach 함수호출을 통해 종료된 쓰레드가 메모리에서 완전히 소멸되게 함
       pthread_detach(t_id); // detach ! pthread_join() 할 필요 X
       printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}
   
void * handle_clnt(void * arg)
{
   int clnt_sock= *( (int*)arg );
   int str_len=0, i;
   char msg[BUF_SIZE];
   
   while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0) // 인자로 전달된 클라이언트 파일디스크립터에게
                                                         // 메세지를 받으면 read를 하며 클라이언트가 연결 종료 할 때까지 메세지 전송 반복
      send_msg(msg, str_len); // 읽은 메세지를 send_msg()함수로 전달
     
   pthread_mutex_lock(&mutx); // 클라이언트 소켓fd 를 제거하기 위한 Mutex
   for(i=0; i<clnt_cnt; i++)   // remove disconnected client
   {
      if(clnt_sock==clnt_socks[i]) // clnt_sock값과 해당 클라이언소켓 배열에 있는 클라이언트 파일 디스크립터 값과 동일하면
      {
         while(i++ < clnt_cnt-1)
            clnt_socks[i]=clnt_socks[i+1]; // 한 칸씩 앞으로 떙겨주며 제거
         break;
      }
   }
   clnt_cnt--; // 클라이언트 소켓이 하나 제거됐으므로 count값 내림
   pthread_mutex_unlock(&mutx); // 공유변수에 대한 처리를 끝냈으므로 unlock
   close(clnt_sock);
   return NULL;
}
void send_msg(char * msg, int len)   // send to all
{
   int i;
   pthread_mutex_lock(&mutx); // 클라이언트 소켓 fd로 write하기 위한 mutex
   // 현재 연결된 모든 클라이언트들에게 방금 연결된 클라이언트가 보낸 메세지를 뿌려줌
   for(i=0; i<clnt_cnt; i++)
      write(clnt_socks[i], msg, len);
   pthread_mutex_unlock(&mutx); // unlock
}
void error_handling(char * msg)
{
   fputs(msg, stderr);
   fputc('\n', stderr);
   exit(1);
}