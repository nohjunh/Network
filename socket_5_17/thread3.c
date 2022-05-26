#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void * thread_summation(void * arg); 

int sum=0;

int main(int argc, char *argv[])
{
    pthread_t id_t1, id_t2;
    int range1[]={1, 5}; // int형 배열
    int range2[]={6, 10}; // int형 배열
    
    pthread_create(&id_t1, NULL, thread_summation, (void *)range1); // 배열을 스타트루틴 함수 인자 값으로 전달
    pthread_create(&id_t2, NULL, thread_summation, (void *)range2);

    pthread_join(id_t1, NULL); // id_t1에 저장된 ID의 쓰레드가 종료 될때까지 프로세스 종료 대기
    pthread_join(id_t2, NULL);
    printf("result: %d \n", sum);
    return 0;
}

void * thread_summation(void * arg) 
{
    int start=((int*)arg)[0]; // 인자로 들어온 range1배열의 첫 번째 요소 int형으로 형변환 후 참조
    int end=((int*)arg)[1];// 인자로 들어온 range2배열의 두 번째 요소 int형으로 형변환 후 참조

    while(start <= end)
    {
        sum += start; // sum이라는 global변수에 쓰레드 2개가 동시에 접근 -> 동기화문제 해결방안 중요
        start++;
    }
    return NULL;
}