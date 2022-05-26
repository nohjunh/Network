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

#define BUF_SIZE 100
#define PACKETSIZE	64
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
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
void ping(struct sockaddr_in *addr) // 핑에서 보냄
{	const int val=240; // TTL 값을 240으로 설정
	int i, sd, cnt=0; // sequence 값은 2부터 시작해야 하므로 cnt를 0로 하고 보낼때마다 +2한 값을 넣어주고 시작하면 2부터 시작.
	struct packet pckt;
	struct sockaddr_in r_addr;
	int bytes;
    // ICMP 메세지를 보낼 수 있는 RAW socket 생성
	sd = socket(PF_INET, SOCK_RAW, proto->p_proto); // RAW socket 열때는 SOCK_RAW로 함. // RAW socket 타입 + ICMP 프로토콜 
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0) // 소켓에 대한 옵션 설정, SOL_IP: IP 레벨의 옵션을 바꿈 / IP_TTL: IP 레벨의 TTL 옵션을 바꿈->
	                                                  			// val 값으로 IP 패킷의 TTL 설정(=240)으로 바꾼다.
		perror("Set TTL option");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0) // Non-Blocking 소켓 설정
		perror("Request nonblocking I/O");

    //////////////////////////
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char opCount, operator; // opCount를 char형태로 저장
    int operand[1024];    
    memset(operand, 0, sizeof(operand));

    if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket creation failed");
    }

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY); // UDP 소케은 INADDR_ANY와 8000번 포트로 바인드시킴
    servaddr.sin_port = htons(atoi("8000"));

    if( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){ // 소켓 바인드 진행
        perror("bind failed");
    }

    char tokenstring[BUF_SIZE];
    char calc_msg[BUF_SIZE];

	for (;;)
	{	
		int len=sizeof(r_addr);
		int opResult;
		struct packet pckt;
		memset(operand, 0, sizeof(operand));

		bzero(&pckt, sizeof(pckt)); // 패킷 내부를 0으로 초기화
		pckt.hdr.type = 20; 	// type을 20번으로 설정
		// Echo request 할때는 8, Echo reply 할때는 0 으로 맵핑됨.
		pckt.hdr.un.echo.id = pid;	// process ID 값을 그대로 넣어줌.

        memset(tokenstring, 0, sizeof(tokenstring));
        memset(calc_msg, 0 ,sizeof(calc_msg));
        fgets(tokenstring, BUF_SIZE, stdin); // 띄어쓰기를 기준으로 키보드를 통해 계산 요청 데이터를 입력 받음
        strcpy(calc_msg, tokenstring);
        char *ptr = strtok(tokenstring, " ");
        int opCount= atoi(ptr);
        char overflowCheck = (char)opCount;
        // Char 기준 오버플로우가 발생하는 연산자 수를 받을 경우,
        if(overflowCheck <= 0) 
        {
            printf("Overflow Number(%d) - Closed client\n", overflowCheck); // 다음 문자열을 표준 출력하고, 
            memset(tokenstring, 0, sizeof(tokenstring));
            close(sockfd); // UDP 소켓을 닫음
            close(sd); // RAW 소켓을 닫음

            exit(0);
        }

		for (int i = 0; i < strlen(calc_msg); i++ ) {
			pckt.msg[i] = calc_msg[i];
		}
		pckt.msg[strlen(pckt.msg)+1] = '\0';

		pckt.hdr.un.echo.sequence = (cnt=cnt+2); // 메세지를 보낼 때마다 count를 늘릴건데, 이 값을 그대로 sequence에 넣음.
										   // ICMP 메세지 별로 sequence 값이 2->4->6 ...... 이렇게 저장됨.
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt)); // checkSum은 패킷이 중간에 손상돼서 왔는지에 대한 오류check를 함.
		// sendto 부분
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");
		usleep(1000);

        len=sizeof(cliaddr); // socket address structure size
        recvfrom(sockfd, &opResult, 4, 0, (struct sockaddr*)&cliaddr, &len);
    
        printf("Result: %d\n", opResult); // 표준출력으로 Operation result: 와 함께 출력하고 소켓을 닫고 종료
	}

    close(sockfd); // UDP 소켓 닫음.
}

/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/

int main(int count, char *strings[])
{	struct hostent *hname;
	struct sockaddr_in addr;

	if ( count != 2 )
	{
		printf("usage: %s <addr>\n", strings[0]);
		exit(0);
	}
	if ( count > 1 )
	{
		pid = getpid(); // getpid()를 통해 자신의 process id를 얻고,
        // getprotobyname()을 통해 ICMP의 프로토콜 정보를 protoent 구조체에 저장함.
		proto = getprotobyname("ICMP"); // 프로토콜 명 -> protoent structure의 p_proto 변수에 ICMP의 프로토콜 넘버가 들어간다.
		hname = gethostbyname("127.0.0.1"); // DNS를 통해 정해준 127.0.0.1 를 IP 주소로 넣음.
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0; // Port는 초깃값 0 !
		addr.sin_addr.s_addr = *(long*)hname->h_addr; // addr 구조체에 값을 넣어줌.
	
		ping(&addr); // 부모 프로세스에서는 얻은 IP주소로 ping을 보냄.
		wait(0);

	}
	else
		printf("usage: myping <hostname>\n");
	return 0;
}
