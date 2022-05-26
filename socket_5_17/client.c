#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/uio.h> // writev, readv
   
#define BUF_SIZE 100
   
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
   
char ID[BUF_SIZE];
char msg[BUF_SIZE];
   
int main(int argc, char *argv[])
{
   int sock;
   struct sockaddr_in serv_addr;
   pthread_t snd_thread, rcv_thread;
   void * thread_return;

   if(argc!=4) {
      printf("Usage : %s <port> <IP> <ID 4글자>\n", argv[0]);
      exit(1);
    }
   // ID가 4글자가 아니라면 "ID have to be 4" 메세지 출력 후 종료
   if(strlen(argv[3]) != 4 ){
       printf("ID have to be 4\n");
       exit(0);
   }
   sprintf(ID, "[%s]", argv[3]); // argv[3]으로 받은 값이 ID이므로 ID배열에 문자열로 저장
  
   sock=socket(PF_INET, SOCK_STREAM, 0); // 클라이언트 소켓 생성

   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family=AF_INET;
   serv_addr.sin_addr.s_addr=inet_addr(argv[2]); // ip
   serv_addr.sin_port=htons(atoi(argv[1])); // port

    // 서버와 연결하고 멀티스레드 형식으로 send 및 recv 하는 스레드들 생성
   if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
      error_handling("connect() error");
   
   pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); // send용 스레드
   pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); // recv용 스레드
   // pthread_join으로 쓰레드 종료대기
   pthread_join(snd_thread, &thread_return);
   pthread_join(rcv_thread, &thread_return);
   close(sock);  
   return 0;
}

// send스레드의 경우 다음과 같이 동작
void * send_msg(void * arg)   // send thread main
{
    // iovec 구조체를 활용해 1번째배열 -> main함수 인자로 얻은 ID 입력받고
    //                      2번째배열 -> 계산 요청 데이터 입력 받아서 writev()를 통해 데이터 전송
    struct iovec vec[2];

    int sock=*((int*)arg);
    char calc_msg[BUF_SIZE];
    char tokenstring[BUF_SIZE];
    // iov_base는 버퍼의 주소 정보
    // iov_len은 버퍼의 크기정보
    vec[0].iov_base= ID;
    vec[0].iov_len= 6; // 4글자의 ID값 + '[', ']' 해서 총 6을 보냄
    vec[1].iov_base= calc_msg;
    vec[1].iov_len= BUF_SIZE;
    

    while(1)
    {
      memset(tokenstring, 0, sizeof(tokenstring));
      memset(calc_msg, 0 ,sizeof(calc_msg));

      fgets(tokenstring, BUF_SIZE, stdin);
      strcpy(calc_msg, tokenstring);
      char *ptr = strtok(tokenstring, " ");
      int opCount= atoi(ptr);
      char overflowCheck = (char)opCount;
      if(overflowCheck <= 0) 
      {
        // 첫 번째 인자의 경우 사용하지 않으므로 특정한 값을 넣지 않고 writev로 전달
        vec[0].iov_len= 0; 
        vec[1].iov_base= ptr;
        vec[1].iov_len= strlen(ptr);
        writev(sock, vec, 2);
        printf("Overflow Number(%d) - Closed client\n", overflowCheck);
        memset(tokenstring, 0, sizeof(tokenstring));
        memset(calc_msg, 0 ,sizeof(calc_msg));
        close(sock);
        exit(0);
      }
      writev(sock, vec, 2); // 문자열 내용을 서버에게 전달
      
    }
    return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
   int sock= *( (int*)arg ); // 소켓 파일 디스크립터
   char result_msg[BUF_SIZE];
   memset(result_msg, 0, sizeof(result_msg));
   int str_len;
   while(1)
   {
      str_len=read(sock, result_msg, BUF_SIZE); // 서버에게 받은 내용을 읽고
      if(str_len==-1) 
         return (void*)-1;
      if(strcmp(result_msg, "end") == 0){ 
        break;
      }
      printf("%s\n", result_msg); // 표준출력으로 값을 뿌림.
      memset(result_msg, 0, sizeof(result_msg));
   }
   return NULL;
}
   
void error_handling(char *msg)
{
   fputs(msg, stderr);
   fputc('\n', stderr);
   exit(1);
}