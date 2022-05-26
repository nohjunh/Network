#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    int status;
    pid_t pid = fork();

    if(pid==0){
        return 3; // 첫번째 자식 리턴값 바로 리턴
    }
    else{ // 부모 프로세스
        printf("Chlid PID: %d\n", pid);
        pid=fork();
        if(pid==0){  // 두번째 자식 프로세스 
            exit(7); // 두번째 자식은 7 반환
        }else{ // 부모 프로세스
            printf("Child PID: %d\n", pid);
            wait(&status); // 자식이 종료될 때까지 Block
            if(WIFEXITED(status)){ // 자식이 정상 종료하면 true
                printf("Child send one: %d \n", WEXITSTATUS(status)); // 정상 종료시 상태출력 리턴값
            }
            wait(&status);
            if(WIFEXITED(status)){
                printf("Child send two: %d \n", WEXITSTATUS(status)); // 즉, 종료돤 자식이 반환한 값 출력
            }
            sleep(30);
        }
    }
    return 0;
}