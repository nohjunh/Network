
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

void keycontrol(int sig){
    if(sig==SIGINT){
        puts("CTRL+C pressed");
    }
}


int main(int argc, char *argv[]){
    int i;
    signal(SIGALRM, timeout); // 해당 시그널넘버 발생시 timeout 함수 실행
    signal(SIGINT, keycontrol); // 첫번째 매개변수에 해당하는 시그널 발생시 두 번째 매개변수에 적힌 함수 호출
    alarm(2);

    for(i=0; i<3; i++){
        puts("Wait...");
        sleep(100);
    }
    return 0;
}
