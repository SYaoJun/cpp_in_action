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
#define N 5
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
void my_handler(int sig) {
    // 可能同时有多个子进程结束
    int status = 0;
    pid_t pid;
    // (pid = waitpid(-1, &status, WNOHANG)) != -1)
    while((pid = wait(&status)) != -1) {
        if (WIFEXITED(status)) {
            printf("==========Child exited with status %d.\n", WEXITSTATUS(status));
        }else if(WIFSIGNALED(status)) {
            printf("Child exited with signal %d.\n", WTERMSIG(status));
        }else {
            printf("Child terminated abnormally.\n");
        }
    }
}
int main() {
    // 借助内核回收子进程
    // 先屏蔽信号
    sigset_t mask, prev;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    // 已经设置了信号屏蔽字

    struct sigaction action;
    action.sa_handler = my_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &action, NULL);
    int i;
    pid_t pid;
    for(i = 0; i < N; i++) {
        pid = fork();
        if(pid == 0) {
            break;
        }
    }
    if(i == N) {
        // 父进程
        // 注册信号处理函数

        // 为什么sleep放在前面就不能正确回收呢？
        sleep(2);
        // 解除阻塞，即使父进程多睡眠几秒，也会正常回收子进程
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        printf("I am parent %d\n", getpid());
        // 这里一定要阻塞等待
        while(1);
    }else{
        // 子进程
//        sigprocmask(SIG_UNBLOCK, &mask, NULL);
//        sigprocmask(SIG_SETMASK, &prev, NULL);
//        sleep(2);
        printf("I am child %d\n", getpid());
        exit(i);
    }
    return 0;
}