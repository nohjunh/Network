#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void timeout(int sig){
    if(sig==SIGALRM){
        puts("Time out!");
    }
    alarm(2);
}


int main(int argc, char *argv[]){
    int i;
    struct sigaction act;
    act.sa_handler= timeout; // sa_handler에 호출할 함수 저장
    sigemptyset(&act.sa_mask); // sa_mask랑 flags를 0으로 초기화
    act.sa_flags=0;
    sigaction(SIGALRM, &act, 0); // 세번 째 인자는 일단은 그냥 0으로 채워넣음


    alarm(2);

    for(i=0; i<3; i++){
        puts("Wait...");
        sleep(100);
    }
    return 0;
}
