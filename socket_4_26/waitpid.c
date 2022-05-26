#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    int status;
    pid_t pid= fork();
    
    if(pid==0){
        sleep(15);
        return 24;
    }
    else{
        while(!waitpid(-1, &status, WNOHANG)){ // Non-Block상태로 임의의 자식 프로세스 종료대기. Blocking상태로 쭉 대기하지 않고 자식 프로세스의 종료상태 체크 가능
            sleep(3);
            puts("sleep 3sec");
        }
        if(WIFEXITED(status)){
            printf("Child send %d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}