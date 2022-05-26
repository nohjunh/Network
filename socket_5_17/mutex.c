// 한 쓰레드가 한 임계변수에 접근해서 연산을 완료할 때까지,
// 다른 쓰레드가 해당 변수에 접근하지 못하도록 막아야 하는데,
// 이것이 바로 동기화.
// 일반적으로 임계영역은 쓰레드에 의해서 실행되는 함수 내에 존재

// 쓰레드 동기화-> 쓰레드 접근순서에 의한 문제를 해결
// 쓰레드의 실행순서 컨트롤을 하기 위해 뮤텍스/세마포어 이용
// 뮤텍스: 쓰레드의 동시접근을 허용하지 않는다는 의미, 임계영역에 대한 자물쇠 역할을 함.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREAD	100

void * thread_inc(void * arg);
void * thread_des(void * arg);

long long num=0; // 전역변수 num을 2개의 스레드가 동시에 접근
pthread_mutex_t mutex;

int main(int argc, char *argv[]) 
{
    pthread_t thread_id[NUM_THREAD]; // 쓰레드를 여러 개 만들 것이므로
                                     // 스레드를 관리하기 위한 pthread_t형 배열 선언
    int i;
    
    pthread_mutex_init(&mutex, NULL); // init함수를 통해 mutex 초기화, 성공 시 0 반환

    for(i=0; i<NUM_THREAD; i++)
    {
        if(i%2) // 짝수이면 thread_inc 스타트루틴 함수 실행
            pthread_create(&(thread_id[i]), NULL, thread_inc, NULL);
        else    // 홀수이면 thread_des 스타트루틴 함수 실행
            pthread_create(&(thread_id[i]), NULL, thread_des, NULL);	
    }	

    for(i=0; i<NUM_THREAD; i++) // 생성된 쓰레드가 모두 종료 될 때까지 프로세스가 종료되지 않게 하는 구문
        pthread_join(thread_id[i], NULL);

    printf("result: %lld \n", num);
    pthread_mutex_destroy(&mutex); // Mutex 소멸
    return 0;
}

void * thread_inc(void * arg) 
{
    int i;
    pthread_mutex_lock(&mutex); // 임계영역에 들어가기 전에 호출,
                                // 다른 쓰레드가 임계영역 실행중이면,
                                // 그 쓰레드가 unlock함수를 호출할 때까지(임계영역에서 나올 때까지)
                                // 블로킹

    for(i=0; i<50000000; i++)
        num+=1;

    pthread_mutex_unlock(&mutex); // 임계영역을 나오고 나서 호출
    return NULL;
}
void * thread_des(void * arg)
{
    int i;
    for(i=0; i<50000000; i++)
    {
        pthread_mutex_lock(&mutex); // 임계영역 시작

        num-=1;

        pthread_mutex_unlock(&mutex);  // 임계영역 끝
    }
    return NULL;
}
