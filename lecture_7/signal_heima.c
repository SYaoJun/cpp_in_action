#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
// 信号的操作
void print_set(sigset_t *sigset) {
    for(int i = 0; i < 32; i++) {
        int t = sigismember(sigset, i);
        if(t) {
            putchar('1');
        }else{
            putchar('0');
        }
    }
    printf("\n");
}
int main() {
    sigset_t cur_set, old_set, pending_set;
    //清空
    sigemptyset(&cur_set);
    // 设置阻塞位
    sigaddset(&cur_set, SIGINT);
    sigaddset(&cur_set, SIGBUS);
    int return_code;
    // 取代
    return_code = sigprocmask(SIG_BLOCK, &cur_set, &old_set);
    if(return_code == -1) {
        printf("sigprocmask error!\n");
    }
    while(1) {
        return_code = sigpending(&pending_set);
        if(return_code == -1) {
            printf("sigpending error!\n");
        }
        print_set(&pending_set);
        sleep(30);
        sigpending(&pending_set);
        print_set(&pending_set);
        sigprocmask(SIG_UNBLOCK, &cur_set, NULL);
        print_set(&pending_set);
    }
    return 0;
}