/*
바이너리 세마포어(0과 1을 사용)를 대상으로 쓰레드의 실행순서를 컨트롤함.
sem_init() 함수가 호출되면 운영체제에 의해서 세마포어 오브젝트가 생성
이곳에는 세마포어 값이라 불리는 정수가 하나 기록
이 값은
    sem_post가 호출되면 +1 되고
    sam_wait가 호출되면 -1이 된다.
     하지만 세마포어 값은 0보다 작아질 수 없기 때문에 
     현재 0인 상태에서 sem_wait가 호출되면 
     쓰레드는 함수가 반환되지 않아서 블로킹 사태에 들어간다. 
     다른 쓰레드에서 해당 세마포어 변수에 대해 sem_post를 호출하면 값이 1로 바뀌고,
     블로킹 상태에서 빠져나간다.
*/
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

void * read(void * arg);
void * accu(void * arg);
static sem_t sem_one; // 세마포어의 참조값을 저장하고 있는 변수
static sem_t sem_two; // 세마포어의 참조값을 저장하고 있는 변수
static int num;

int main(int argc, char *argv[])
{
    pthread_t id_t1, id_t2; // 쓰레드 ID 저장을 위한 pthread_t 변수들
    
    // sem_init의 첫 번째 인자는 세마포어 생성시 세마포어 참조값 저장을 위한 변수
    //            두 번째 인자는 속성값인데 안쓰므로 0으로 하면 됨
    //            세 번째 인자는 세마포어 카운터. 0이면 해당 임계영역에 진입불가,
    //                                           0보다 크면 진입가능
    sem_init(&sem_one, 0, 0); 
    sem_init(&sem_two, 0, 1); // sem_two가 처음에 카운터가 1이므로 wait를 통해 임계영역에 혼자 접근 가능

    pthread_create(&id_t1, NULL, read, NULL);
    pthread_create(&id_t2, NULL, accu, NULL);

//// 쓰레드가 종료 될때까지 프로세스의 종료대기
    pthread_join(id_t1, NULL); 
    pthread_join(id_t2, NULL);

    sem_destroy(&sem_one); // 세마포어 소멸
    sem_destroy(&sem_two);
    return 0;
}

void * read(void * arg)
{
    int i;
    for(i=0; i<5; i++)
    {
        fputs("Input num: ", stdout);

        // 시작 시 0보다 크므로 sem_two를 -1하고 진입 ->
        sem_wait(&sem_two); // sem_two의 세마포어 값 0이 되고 해당 임계영역 시작
        scanf("%d", &num);

        // 키보드 입력을 받고 sem_one을 +1 ->
        sem_post(&sem_one); // sem_one의 세마포어 값을 1로 증가시켜 1로 만들고 해당 임계영역 끝
    }
    return NULL;	
}
void * accu(void * arg)
{
    int sum=0, i;
    for(i=0; i<5; i++)
    {
        // 시작 시 0이므로 대기하다가, 
        sem_wait(&sem_one); // read() 스타트루틴 스레드가 sem_one +1을 하면 블로킹에서 벗어나
                            // -1의 값을 주어 0으로 바꾸고 임계영역에 진입 
        sum+=num;

        sem_post(&sem_two); // sum에 키보드로 입력받은 num값 누적 -> sem_two 를 +1
    }
    printf("Result: %d \n", sum);
    return NULL;
}