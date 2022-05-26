#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#define BUF_SIZE 30

// select함수를 사용하면 한 곳에 여러 개의 파일 디스크립터를 모아놓고 관찰 가능
// 파일 디스크립터를 묶음을 모을때 fd_set형 구조체 변수를 사용
// 비트가 1로 설정되면 해당 파일 디스크립터가 관찰의 대상임을 의미
/*
fd_set형 변수에 값을 등록하거나 변경하는 등의 작업은 다음 매크로 함수들의 도움을 통해 이뤄진다.
- FD_ZERO(fd_set * fdset) : 인자로 전달된 주소의 fd_set형 번수의 모든 비트를 0으로 초기화
- FD_SET(int fd, fd_set *fdset) : 매개변수 fdset으로 전달된 주소의 변수에 매개변수 fd로 전달된 파일 디스크립터 정보를 등록
- FD_CLR(int fd, fd_set *fdset) : 매개변수 fdset으로 전달된 주소의 변수에서 매개변수 fd로 전달된 파일 디스크립터 정보를 삭제한다.
- FD_ISSET(int fd, fd_set *fdset) : 매개변수 fdset으로 전달된 주소의 변수에서 매개변수 fd로 전달된 파일 디스크립터 정보가 있으면 양수를 반환한다.
위의 함수들 중에서 FD_ISSET은 select 함수의 호출결과를 확인하는 용도로 사용
*/

int main(int argc, char *argv[])
{
   fd_set reads, temps;     
   int result, str_len;
   char buf[BUF_SIZE];
   struct timeval timeout;

   FD_ZERO(&reads); // fd_set형 번수인 reads의 모든 비트를 0으로 초기화
   FD_SET(0, &reads);  // "0 is standard input(console)" 지정된 값
                       // 0번 파일디스크립터(표준입력)를 fd_set형 변수인 reads에 등록
                       // 즉, 키보드에 이벤트가 발생하는지 관찰하겠다는 의미.

   while(1)
   {
      temps = reads; // reads의 정보를 temps에 저장
      // select 함수는 관찰중인 파일 디스크립터에 변화가 생겨야 반환을 한다. 
      // 때문에 변화가 생기지 않으면 무한정 블로킹 상태에 머물게 된다.
      // 초단위와 마이크로 초 단위 정보를 지정하고 select를 호출할 때 전달하면 된다.
      timeout.tv_sec = 5; 
      timeout.tv_usec = 0;

      // select 함수의 반환값이 0이 아닌 양수가 반환이 되면, 
      // 그 수만큼 fd에 변화가 발생했음을 의미한다. 
      // 여기서의 변화는 등록된 fd에 해당 관심에 관련된 변화가 생겼음을 의미한다.
      // 즉, select 함수의 두 번째 인자를 통해서 데이터 수신여부의 관찰대상에 포함된 
      // fd로 수신된 데이터가 존재하는 경우가 fd에 변화가 발생한 경우이다.
      // fd_set을 select의 인자로 전달했는데, 변화가 발생한 fd에 해당하는 비트는
      // 그대로 1로 남아있고 나머지는 0으로 바뀐 것을 가지고 fd에 변화가 발생했다고
      // 판단할 수 있다. 즉, select 함수호출이 완료되고 나면, 
      // select 함수의 인자로 전달된 fd_set형 변수에는 변화가 생긴다. 
      // 1로 설정된 모든 비트가 다 0으로 변경되지만,
      // 변화가 발생한 파일 디스크립터에 해당하는 비트만 그대로 1로 남아있게 된다.
      // 때문에 여전히 1로 남아있는 위치의 파일 디스크립터에서 변화가 발생했다고
      // 판단할 수 있다.

      //int select(int maxfd, fd_set *readset, fd_set *sriteset, fd_set *exceptest, const struct timeval * timeout);
      // maxfd : 검사 대상이 되는 파일 디스크립터 수
      // readset : fd_set형 변수에 '수신된 데이터의 존재여부'에 관심 있는 파일 디스크립터 정보를 
      // 모두 등록해서 그 변수의 주소값을 전달한다.
      // writeset : fd_set형 변수에 '블로킹 없는 데이터 전송의 가능여부'에
      // 관심있는 파일 디스크립터 정보를 모두 등록해서 그 변수의 주소값을 전달한다.
      // exceptset : fd_set형 변수에 '예외상황의 발생여부'에 관심이 있는 파일 디스크립터
      // 정보를 모두 등록해서 그 변수의 주소 값을 전달한다.
      // timeout : select 함수 호출 이후에 무한정 블로킹 상태에 빠지지 않도록 타임아웃(time-out)을 설정하기위한 인자를 전달한다.
      // **************************************************
      // 반환 값 : 오류 발생시 -1이 반환되고, 
      // 타임 아웃에 의한 반환 시에는 0이 반환된다.
      // 그리고 관심대상으로 등록된 파일 디스크립터에 해당 관심에 관련된 변화가 발생하면
      // 0보다 큰 값이 반환되는데, 이 값은 변화가 발생한 파일 디스크립터의 수 의미.

      result = select(1, &temps, 0, 0, &timeout);
      if(result == -1) // select함수의 반환값이 음수이면 에러
      {
         puts("select() error!");
         break;
      }
      else if(result == 0) // select함수의 반환값이 0 이면 
                           // 관찰중인 파일 디스크립터에 변화가 생기지 않았다는 뜻
      {
          //실행하고 나서 아무런 입력이 없으면 5초 정도 지나서 타임아웃이 발생
         puts("Time out!");
      }
      else
      {
         // 키보드로 문자열을 입력하면 해당 문자열이 재출력됨.
         // 서버에서 select함수 이후 FD_ISSUE를 통해서 원하는 이벤트가 들어왔는지 판단
         // FD_ISSET 함수를 통해,
         // fd_set형 변수 temps에 등록된 파일디스크립터 중에서 
         // 0번 파일디스크립터로 전달된 정보가 있으면 양수를 반환
         if(FD_ISSET(0, &temps))  // 즉, 0번 파일디스크립터(stdin:표준입력)에서 이벤트가 발생하였는가?
         {
            str_len = read(0, buf, BUF_SIZE); // 첫번째 인자: 0번 파일디스크립터의 소켓
                                              // '표준입력'으로 들어온 data를 buf에 저장
            buf[str_len] = 0;
            printf("message from console : %s", buf);
         }
      }
   }
   return 0;
}