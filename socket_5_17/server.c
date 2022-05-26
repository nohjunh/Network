#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/uio.h> // writev, readv

#define BUF_SIZE 100

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

// 공유변수로 사용됨. 즉, 임계영역을 구성 -> 현재 연결된 클라이언트 수를 세는 용도의 변수들
int clnt_cnt=0;  // 서버에 접속한 클라이언트의 소켓들을 관리하는 변수
int clnt_socks[BUF_SIZE]; 
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
    serv_adr.sin_port=htons(atoi(argv[1])); // port번호를 인자로부터 받아서 서버소켓 구성
   
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
       clnt_socks[clnt_cnt++]= clnt_sock; // clnt_socks와 clnt_cnt는 공유변수이므로 처리 과정중에
                                         // mutex를 통해 다른 스레드가 접근하지 못하도록 함.
                                         // 파일 디스크립터 값이 배열에 저장될 것이다.
       pthread_mutex_unlock(&mutx); // unlock
   
       pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); // 연결된 클라이언트 파일 디스크립터 번호를 스타트루틴의 인자로 전달
       //detach 함수호출을 통해 종료된 쓰레드가 메모리에서 완전히 소멸되게 함
       pthread_detach(t_id); // detach ! pthread_join() 할 필요 X

       // detach 후 클라이언트 포트번호를 표준 출력한 뒤, 다음 연결 요청 대기
       printf("Connected client Port: %d \n", clnt_adr.sin_port);
    }
    close(serv_sock);
    return 0;
}
   
void * handle_clnt(void * arg)
{
   int clnt_sock= *( (int*)arg );
   
    while(1){// 인자로 전달된 클라이언트 파일디스크립터에게
             // 메세지를 받으면 read를 하며 클라이언트가 연결 종료 할 때까지 메세지 전송 반복
        char result[BUF_SIZE];
        struct iovec vec[2];
        char ID_buf[BUF_SIZE]={0, };
        char calc_data_buf[BUF_SIZE]={0, };
        int str_len=0;
        vec[0].iov_base= ID_buf;
        vec[0].iov_len=6; // 첫번째 배열에는 4글자의 id값과 '[', ']' 를 합쳐서 6을 받음
        vec[1].iov_base= calc_data_buf; // 두 번 째 배열에는 계산 데이터를 줌.
        vec[1].iov_len= BUF_SIZE; // 버퍼에 있는 값을 다 읽어라.
                                                
        char opCount, operator; // opCount를 char형태로 저장
        int operand[1024];
        memset(operand, 0, sizeof(operand));
        memset(result, 0, sizeof(result));

        str_len=readv(clnt_sock, vec, 2);


        sprintf(result, "%s", ID_buf); // int값 char배열에 저장
        strcat(result, " ");
        
        char *ptr = strtok(calc_data_buf, " ");

        opCount= calc_data_buf[0]-48;
        char checkoverflow= (char)opCount;
        if(checkoverflow<=0){
            write(clnt_sock, "end", 3);
            break;
        }

        for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
            ptr= strtok(NULL, " ");
            operand[i]= atoi(ptr); // char형을 int형으로
        }
        
        int calc_result = operand[0]; // 시작값 index 0 부터
        char temp2[BUF_SIZE];
        memset(temp2, 0, sizeof(temp2));
        sprintf(temp2, "%d", operand[0]); // int값 char배열에 저장
        strcat(result, temp2);
        for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
            ptr= strtok(NULL, " ");
            operator= ptr[0];
            switch(operator){
            case '+':
                calc_result+=operand[i+1]; // 시작값이 index 0 부터 시작이므로 그 다음 operand부터 잡아야한다. 따라서 i+1
                strcat(result, "+");
                sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                strcat(result, temp2);
                break;
            case '-':
                calc_result-=operand[i+1];
                strcat(result, "-");
                sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                strcat(result, temp2);
                break;
            case '*':
                calc_result*=operand[i+1];
                strcat(result, "*");
                sprintf(temp2, "%d", operand[i+1]); // int값 char배열에 저장
                strcat(result, temp2);
                break;
            default:
                break;
            }
        }
        strcat(result, "=");
        sprintf(temp2, "%d", calc_result); // int값 char배열에 저장
        strcat(result, temp2); // [id]계산식 끝
        //printf("%s\n", result);
        send_msg(result, strlen(result)); // 계산 결과를 send_msg()함수로 전달
   }

   pthread_mutex_lock(&mutx); // 클라이언트 소켓fd 를 제거하기 위한 Mutex
   for(int i=0; i < clnt_cnt; i++)   // remove disconnected client
   {
      if(clnt_sock == clnt_socks[i]) // clnt_sock값과 해당 클라이언트 소켓 배열에 있는 클라이언트 파일 디스크립터 값과 동일하면
      {
         while(i < clnt_cnt-1){
            clnt_socks[i]=clnt_socks[i+1]; // 한 칸씩 앞으로 떙겨주며 제거
            i++;
         }
         break;
      }
   }
   clnt_cnt--; // 클라이언트 소켓이 하나 제거됐으므로 count값 내림
   pthread_mutex_unlock(&mutx); // 공유변수에 대한 처리를 끝냈으므로 unlock
   close(clnt_sock);
   printf("closed client\n"); // 클라이언트가 닫혔음을 알려줌.
   return NULL;
}

void send_msg(char * result, int len)   // send to all
{
   int i;
   pthread_mutex_lock(&mutx); // 클라이언트 소켓 fd로 write하기 위한 mutex
   // 현재 연결된 모든 클라이언트들에게 방금 연결된 클라이언트가 보낸 메세지를 뿌려줌
   for(i=0; i<clnt_cnt; i++)
      write(clnt_socks[i], result, len);
   pthread_mutex_unlock(&mutx); // unlock
}

void error_handling(char * msg)
{
   fputs(msg, stderr);
   fputc('\n', stderr);
   exit(1);
}