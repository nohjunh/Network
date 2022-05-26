#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void* thread_main(void *arg);

int main(int argc, char *argv[]) 
{
    pthread_t t_id;
    int thread_param=5;   // 스레드에 start루틴함수 인자값으로 5을 보냄.
    // 스타트루틴은 쓰레드의 main 함수역할을 하는 별도 실행흐름이 시작되는 함수
    // t_id는 생성할 쓰레드의 ID 저장을 위한 변수의 주소 값이다.
    // 참고로 쓰레드는 프로세스와 마찬가지로 쓰레드의 구분을 위한 ID가 부여됨.
    if(pthread_create(&t_id, NULL, thread_main, (void*)&thread_param) != 0) // 속성값은 그냥 NULL로 함.
    {                                           // void형으로 하는 이유는 나중에 인자를 전달할때 자료형에 구애받지 않고 사용하기 편하게 하기 위함.
        puts("pthread_create() error");
        return -1;
    }; // 세미클론이 들어감.
    sleep(10);
    puts("end of main");
    return 0;
}

void* thread_main(void *arg) 
{
    int i;
    int cnt=*( (int*)arg ); // void형으로 형변환되어서 넘어온 인자를 int형으로 형변환하여 사용.
    for(i=0; i<cnt; i++)
    {
        sleep(1);
        puts("running thread");	 
    }
    return NULL;
}