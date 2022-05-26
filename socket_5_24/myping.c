/* myping.c
 *
 * Copyright (c) 2000 Sean Walton and Macmillan Publishers.  Use may be in
 * whole or in part in accordance to the General Public License (GPL).
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

/*****************************************************************************/
/*** myping.c                                                              ***/
/***                                                                       ***/
/*** Use the ICMP protocol to request "echo" from destination.             ***/
/*****************************************************************************/
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
	struct in_addr addr;

	printf("----------------\n");

	addr.s_addr = ip->saddr; // 소스 주소

	printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d src=%s ",
		ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol, // ip->protocol 값을 1이 찍힐 것이다.
		ip->ttl, inet_ntoa(addr));

	addr.s_addr = ip->daddr; // 내 컴퓨터의 IP 주소 일 것이다. ( 우리가 설정한 echo 이므로 )

	printf("dst=%s\n", inet_ntoa(addr));
	if ( icmp->un.echo.id == pid )	// 내가 보낸게 맞다면, 즉 pid가 같다면 (Echo니까)
	{
		printf("ICMP: type[%d/%d] checksum[%d] id[%d] seq[%d]\n\n",
			icmp->type, icmp->code, ntohs(icmp->checksum),
			icmp->un.echo.id, icmp->un.echo.sequence);
	}
}


/*--------------------------------------------------------------------*/
/*--- listener - separate process to listen for and collect messages--*/
/*--------------------------------------------------------------------*/


void listener(void) // 리스너에서 소켓을 열고 반복적으로 ICMP 메세지를 띄움.
{	int sd;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto); // 소켓을 똑같이 RAW Socket으로 염 ( Raw socket 타입 + ICMP 프로토콜 )
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{	
		int bytes, len=sizeof(addr);

		bzero(buf, sizeof(buf)); // buf 초기화
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len); // buf로 ICMP reply 메세지를 받음.
		// buf에 IP 패킷 ( 헤더 + 데이터 ) 그 자체를 모두 가지고 옴.
		if ( bytes > 0 ) { // 메세지를 성공적으로 받으면 당연히 0보다 클 것이다. 
			printf("***Got message!***\n");
			display(buf, bytes);
		}
		else
			perror("recvfrom");
	}
	exit(0);
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
void ping(struct sockaddr_in *addr) // 핑에서 보냄
{	const int val=255;
	int i, sd, cnt=1;
	struct packet pckt;
	struct sockaddr_in r_addr;
	int bytes;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto); // RAW socket 열때는 SOCK_RAW로 함. // RAW socket 타입 + ICMP 프로토콜 
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0) // 소켓에 대한 옵션 설정, SOL_IP: IP 레벨의 옵션을 바꿈 / IP_TTL: IP 레벨의 TTL 옵션을 바꿈->
	                                                  			// val 값으로 IP 패킷의 TTL 설정(=255)으로 바꾼다.
		perror("Set TTL option");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0) // Non-Blocking 소켓 설정
		perror("Request nonblocking I/O");

	for (;;)
	{	int len=sizeof(r_addr);

		printf("Msg #%d\n", cnt);	// 핑을 몇 번 보냈는지 count	
		bzero(&pckt, sizeof(pckt)); // 패킷 내부를 0으로 초기화
		pckt.hdr.type = ICMP_ECHO; 	// type을 ICMP_ECHO로 설정
		// Echo request 할때는 8, Echo reply 할때는 0 으로 맵핑됨.
		pckt.hdr.un.echo.id = pid;	// process ID 값을 그대로 넣어줌.
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ ) {
			pckt.msg[i] = i+'0'; // 캐릭터형으로 변환해서 배열에 넣어줌.
		}
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++; // 메세지를 보낼 때마다 count를 늘릴건데, 이 값을 그대로 sequence에 넣음.
										   // ICMP 메세지 별로 sequence 값이 1->2->3 ...... 이렇게 저장됨.
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt)); // checkSum은 패킷이 중간에 손상돼서 왔는지에 대한 오류check를 함.
		// sendto 부분
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");
		sleep(1);	
	}
}

/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/

// fork를 통해 하나는 보내기만 하고 하나는 읽기만 하게 만듬.

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
		pid = getpid();
		proto = getprotobyname("ICMP"); // 프로토콜 명 -> protoent structure의 p_proto 변수에 ICMP의 프로토콜 넘버가 들어간다.
		hname = gethostbyname(strings[1]); // DNS를 통해 특정 name을 넣으면 IP 주소를 얻을 수 있다.
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0; // Port는 초깃값 0 !
		addr.sin_addr.s_addr = *(long*)hname->h_addr; // addr 구조체에 값을 넣어줌.
	
		if ( fork() == 0 ) // 프로세스 2개를 만들어 하나는 메세지를 받고 출력, 하나는 핑을 보내는 역할만 한다.
			listener(); //  자식 프로세스에서는 나한테 오는 메세지를 읽음
		else
			ping(&addr); // 부모 프로세스에서는 얻은 IP주소로 ping을 보냄.
		wait(0);

	}
	else
		printf("usage: myping <hostname>\n");
	return 0;
}
