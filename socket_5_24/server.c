#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define PACKETSIZE	64
#define BUF_SIZE 100

struct packet
{
	struct icmphdr hdr; // icmphdr structure = 헤더 structure 에서 type별로 코딩이 가능하도록 구성함 .
	char msg[PACKETSIZE-sizeof(struct icmphdr)]; // 데이터 크기 = 패킷크기 - 헤더 크기
};

int pid=-1; // pid를 담을 변수
struct protoent *proto=NULL; // protoent structure 

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/

unsigned short checksum(void *b, int len) // 전송된 데이터에 오류가 있는지 체크
{	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

/*--------------------------------------------------------------------*/
/*--- display - present echo info                                  ---*/
/*--------------------------------------------------------------------*/
void display(void *buf, int bytes)
{	int i;
	struct iphdr *ip = buf; // ip Header 구조체에 받은 데이터들을 일대일로 맵핑시켜줄 것이다.
							// buf에는 IP header와 IP data가 통째로 들어가 있을 것이다.
	struct icmphdr *icmp = buf+ip->ihl*4; // ihl= head length 이고, 이 식은 전송받은 buf 중 IP header부분을 뛰어넘고 ICMP 메세지 부분만 가리키도록 함.
    struct icmphdr *ss =buf+ip->ihl*4+8; // 포인터값을 수정해 ICMP 메세지 중 계산수식 부분을 읽게 함.
	struct in_addr addr;

	addr.s_addr = ip->saddr; // 소스 주소

	printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d src=%s ", 
		ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol, // ip->protocol 값을 1이 찍힐 것이다.
		ip->ttl, inet_ntoa(addr));

	addr.s_addr = ip->daddr; // 내 컴퓨터의 IP 주소 일 것이다. ( 우리가 설정한 echo 이므로 )

	printf("dst=%s type=%d\n", inet_ntoa(addr),  icmp->type); // 뒤에 type값을 추가로 넣어줌.
	if ( icmp->type == 20 )	// 내가 보낸게 맞다면, 즉 pid가 같다면 (Echo니까)
	{
		printf("ICMP CALC: id[%d] seq[%d]\n",
			icmp->un.echo.id, icmp->un.echo.sequence);
	}

    //////////////////////////////////
    if ( icmp->type == 20 ){	// 내가 보낸게 맞다면, 즉 pid가 같다면 (Echo니까)
        int sockfd;
        struct sockaddr_in servaddr;
        socklen_t len;
        char opCount, operator; // opCount를 char형태로 저장
        int operand[1024];    
        
        memset(operand, 0, sizeof(operand));

        if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
            perror("socket creation failed");
        }

        servaddr.sin_family = AF_INET;
        char temp3[BUF_SIZE];
        strcpy(temp3, inet_ntoa(addr)); // ICMP로 받았던 dest IP와 8000번 포트 설정.
        servaddr.sin_addr.s_addr = inet_addr(temp3); // ICMP로 받았던 dest IP와 8000번 포트 설정.
        servaddr.sin_port = htons(atoi("8000")); // ICMP로 받았던 dest IP와 8000번 포트 설정.

        char calc_data_buf[BUF_SIZE]={0, };
        strcpy(calc_data_buf, (char*)ss); // *ss =buf+ip->ihl*4+8; // 포인터값을 수정해 ICMP 메세지 중 계산수식 부분을 읽게 함.
    
        char *ptr = strtok(calc_data_buf, " ");

        opCount= calc_data_buf[0]-48;

        for(int i=0; i<opCount; i++){ // 클라이언트의 operand count 수만큼 operand 받고,
            ptr= strtok(NULL, " ");
            operand[i]= atoi(ptr);
        }
        
        int calc_result = operand[0]; // 시작값 index 0 부터
        for(int i=0; i<opCount-1; i++){ // (operand count -1) 만큼 연산자 받음
            ptr= strtok(NULL, " ");
            operator= ptr[0];
            switch(operator){
            case '+':
                calc_result+=operand[i+1]; // 시작값이 index 0 부터 시작이므로 그 다음 operand부터 잡아야한다. 따라서 i+1
                break;
            case '-':
                calc_result-=operand[i+1];
                break;
            case '*':
                calc_result*=operand[i+1];
                break;
            default:
                break;
            }
        }
        //printf("Operation result: %d\n", result); // 전송 전에 결과 출력
        usleep(1000);
        sendto(sockfd, &calc_result, 4, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); // 계산된 결과를 클라이언트에게 전송
    
        close(sockfd); // UDP 소켓닫음.
    }
}


/*--------------------------------------------------------------------*/
/*--- listener - separate process to listen for and collect messages--*/
/*--------------------------------------------------------------------*/


void listener(void) // 리스너에서 소켓을 열고 반복적으로 ICMP 메세지를 띄움.
{	int sd;
	struct sockaddr_in addr;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto); // 소켓을 똑같이 RAW Socket으로 염 ( Raw socket 타입 + ICMP 프로토콜 )
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{	
		int bytes, len=sizeof(addr);
		unsigned char buf[1024];

		bzero(buf, sizeof(buf)); // buf 초기화
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len); // buf로 ICMP reply 메세지를 받음.
		// buf에 IP 패킷 ( 헤더 + 데이터 ) 그 자체를 모두 가지고 옴.
		if ( bytes > 0 ) { // 메세지를 성공적으로 받으면 당연히 0보다 클 것이다. 
			display(buf, bytes);
		}
		else
			perror("recvfrom");
	}
	exit(0);
}


/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/

// fork를 통해 하나는 보내기만 하고 하나는 읽기만 하게 만듬.

int main(int count, char *strings[])
{	struct hostent *hname;
	struct sockaddr_in addr;

	pid = getpid(); // getpid()를 통해 자신의 process id를 얻고,
    // getprotobyname()을 통해 ICMP의 프로토콜 정보를 protoent 구조체에 저장함.
	proto = getprotobyname("ICMP"); // 프로토콜 명 -> protoent structure의 p_proto 변수에 ICMP의 프로토콜 넘버가 들어간다.
	hname = gethostbyname("127.0.0.1");
	bzero(&addr, sizeof(addr));
	addr.sin_family = hname->h_addrtype;
	addr.sin_port = 0; // Port는 초깃값 0 !
	addr.sin_addr.s_addr = *(long*)hname->h_addr; // addr 구조체에 값을 넣어줌.
	
	
	listener(); //  자식 프로세스에서는 나한테 오는 메세지를 읽음
	
	wait(0);

	return 0;
}
