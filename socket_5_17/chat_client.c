#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
   
#define BUF_SIZE 100
#define NAME_SIZE 20
   
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
   
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
   
int main(int argc, char *argv[])
{
   int sock;
   struct sockaddr_in serv_addr;
   pthread_t snd_thread, rcv_thread;
   void * thread_return;
   if(argc!=4) {
      printf("Usage : %s <IP> <port> <name>\n", argv[0]);
      exit(1);
    }
   
   sprintf(name, "[%s]", argv[3]); // argv[3]으로 받은 값을 name에 문자열로 저장
   sock=socket(PF_INET, SOCK_STREAM, 0); // 클라이언트 소켓 생성
   
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family=AF_INET;
   serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
   serv_addr.sin_port=htons(atoi(argv[2]));
     
   if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
      error_handling("connect() error");
   
   pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); // send용 스레드
   pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); // recv용 스레드
   // pthread_join으로 쓰레드 종료대기
   pthread_join(snd_thread, &thread_return);
   pthread_join(rcv_thread, &thread_return);
   close(sock);  
   return 0;
}
   
void * send_msg(void * arg)   // send thread main
{
   int sock=*((int*)arg);
   char name_msg[NAME_SIZE+BUF_SIZE];
   while(1) 
   {
      fgets(msg, BUF_SIZE, stdin);
      if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
      {
         close(sock);
         exit(0);
      }
      sprintf(name_msg, "%s %s", name, msg); // name, msg 형태로 문자열을 만들고 서버에게 전송
      write(sock, name_msg, strlen(name_msg)); // 문자열 내용을 서버에게 전달
   }
   return NULL;
}
   
void * recv_msg(void * arg)   // read thread main
{
   int sock= *( (int*)arg ); // 소켓 파일 디스크립터
   char name_msg[NAME_SIZE + BUF_SIZE];
   int str_len;
   while(1)
   {
      str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1); // 서버에게 받은 내용을 읽고
      if(str_len==-1) 
         return (void*)-1;
      name_msg[str_len] = 0;
      fputs(name_msg, stdout); // 표준출력으로 값을 뿌림.
   }
   return NULL;
}
   
void error_handling(char *msg)
{
   fputs(msg, stderr);
   fputc('\n', stderr);
   exit(1);
}