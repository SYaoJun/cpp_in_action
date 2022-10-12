//
// Created by 姚军 on 2022/10/12.
//
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
#include <time.h>
int main() {
    printf("print main, pid = %d.\n", getpid());
    srandom(time(NULL)); // 不管随机种子放在fork之前还是之后，父子都是同样的数值，这是为啥呢？
    pid_t pid = fork();
    int parent = pid != 0;
    int rnd = random();
    if(pid == 0) {
        printf("child rnd = %d\n", rnd);
    }else{
        printf("parent rnd = %d\n", rnd);
    }
    // 0
    //
    if((rnd % 2 == 0) == parent) { // 父子总有一个会睡眠1秒，假设子睡眠一秒，父亲阻塞等待，假设父亲睡一秒，子进程结束
        printf("sleep pid = %d\n", getpid());
        sleep(1);
    }
    if(parent) {
        printf("wait child...\n");
        int status;
        waitpid(pid, &status, 0);
        //meaning wait for any child process whose process group ID is equal to that of the calling process.
        if (WIFEXITED(status)) {
            printf("Child exited with status %d.\n", WEXITSTATUS(status));
        } else {
            printf("Child terminated abnormally.\n");
        }
    }
    printf("print Twice, pid = %d.\n", getpid());
   return 0;
}
