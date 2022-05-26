#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void* thread_main(void *arg);

int main(int argc, char *argv[]) 
{
    pthread_t t_id;
    int thread_param=5;
    void * thr_ret;
    
    if(pthread_create(&t_id, NULL, thread_main, (void*)&thread_param)!=0)
    {
        puts("pthread_create() error");
        return -1;
    }; 	

// pthread_join() 함수는 첫번째 인자로 전달되는 ID의 쓰레드가 종료될 때까지 함수는 반환되지 않는다. 
// => 즉, 메인 프로세스가 쓰레드 종료 시까지종료되지 않게 계속 대기하겠다는 의미
// 두 번째 인자는 스레드의 main함수가 반환하는 값이 저장될 포인터변수의 주소값이 전달됨.
    if(pthread_join(t_id, &thr_ret)!=0) 
    {
        puts("pthread_join() error");
        return -1;
    };

    printf("Thread return message: %s \n", (char*)thr_ret);//포인터변수의 값을 참조해 printf로 출력
    free(thr_ret); // 동적으로 할당된 포인터변수이므로 free로 해제 
    return 0;
}

void* thread_main(void *arg) 
{
    int i;
    int cnt= *( (int*) arg );
    char * msg=(char *)malloc(sizeof(char)*50); // 동적할당
    strcpy(msg, "Hello, I'am thread~ \n");

    for(i=0; i<cnt; i++)
    {
        sleep(1);  puts("running thread");	 
    }
    return (void*)msg; // 동적할당된 msg포인터변수 주소를 리턴
                        // void로 형변환하여 리턴
}