#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num, i;
	char buf[BUF_SIZE];
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock= socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if( bind (serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))== -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)== -1)
		error_handling("listen() error");

    
	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads); // 서버 소켓의 파일 디스크립터를 fd_set형 변수 reads에 등록
                               // 서버(listen) 소켓의 이벤트발생을 감지하겠다는 의미.
	fd_max=serv_sock; // serv_sock의 파일디스크립터 번호를 fd_max에 저장

	while(1)
	{
		cpy_reads=reads; // reads정보를 fd_set형 변수인 cpy_reads에다가 저장
		timeout.tv_sec=5;
		timeout.tv_usec=5000;
        // fd_num은 int형 변수
        // fd의 값은 0부터 시작, 하나씩 생길 때마다 1씩 증가 => 가장 큰 fd + 1를 첫 번째 인자로 전달한다.
        // maxfd는 검사 대상이 되는 파일 디스크립터 수 이므로 maxfd을 넣는 인자자리에 fd_max+1을 해서 지정된 범위까지만 관찰하도록 함.
		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))== -1) // 서버소켓의 파일 디스크립터 번호 +1 까지의 범위 관찰
			break;
		
		if(fd_num==0)
			continue;
        //fd_num의 값이 양수이면 이 값은 변화가 발생한 파일 디스크립터의 수를 의미
		for(i=0; i<fd_max+1; i++) // 0~fd_max+1의 디스크립터를 다 돌면서 FD_ISSET으로 이벤트 발생을 확인
		{
            // fd_set형 변수 cpy_reads에 등록된 파일디스크립터 중에서 
            // i번 파일디스크립터로 전달된 정보가 있으면 양수를 반환 (fd_max까지 다 돌거임)
			if(FD_ISSET(i, &cpy_reads)) 
			{
				if(i==serv_sock) // connection request! 즉, 서버(listen)소켓에서 이벤트가 발생했다면,
				{
					adr_sz=sizeof(clnt_adr);
					clnt_sock=
						accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz); // 클라이언트와 연결
					FD_SET(clnt_sock, &reads); // select를 위해 클라이언트 소켓의 파일 디스크립터 또한 reads에 등록
                                               // 클라이언트 소켓 또한 이벤트 발생을 관찰대상이 됨.
					if(fd_max<clnt_sock) // 클라이언트 소켓까지 관찰할 수 있도록 설정했으니, 파일 디스크립터는
                       fd_max=clnt_sock; // 오름차순으로 생성됨을 고려해, 새로 만들어진 소켓 파일디스크립터까지
                                         // loop 돌기 위한 작업
					printf("connected client: %d \n", clnt_sock); // 연결된 클라이언소켓의 파일 디스크립터 번호 출력
				}
				else // read message!  // i의 값이 클라이언트 소켓 파일디스크립터 번호라면,
				{
					str_len=read(i, buf, BUF_SIZE); // 보내온 data를 읽고,
					if(str_len==0) // close request! // 읽을 데이터가 없다면,
					{
						FD_CLR(i, &reads); // fd_set형 변수 reads에 등록된 i번 파일 디스크립터 정보를 삭제
                                           // 즉, 클라이언트 소켓 등록 해제
						close(i); // 클라이언트 소켓 닫음.
						printf("closed client: %d \n", i); // 닫힌 클라이언트 소켓 번호 출력
					}
					else // 읽을 데이터가 있다면  // 즉, str_len값이 0이 아니라면,
					{
						write(i, buf, str_len); // echo 작업.
					}
				}
			}
		}
	}
	close(serv_sock); // 서버소켓 닫음.
	return 0;
}