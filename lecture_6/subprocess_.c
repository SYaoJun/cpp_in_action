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

typedef struct subprocess_t{
    pid_t pid;
    int supply_fd; //供应
}subprocess_t;

subprocess_t subprocess(const char* command) {
    // 处理传入的命令
    int fds[2];
    pipe(fds);
    subprocess_t process = {fork(), fds[1]}; // 1写 0读
    if (process.pid == 0) {
        close(fds[1]);
        dup2(fds[0], STDIN_FILENO); // 把标准输入重定向为从管道中读
        close(fds[0]);
        char *arguments[] = {"/bin/sh", "-c", (char *) command, NULL};
        execvp(arguments[0], arguments);
        printf("Failed to invoke /bin/sh to execute the supplied command.");
        exit(0);
    }
    close(fds[0]);
    return process;
}
int main(int argc, const char* argv[]) {
    subprocess_t sp = subprocess("sort");
    const char *words[] ={"hello", "world","good","apple"};

    for(size_t i = 0; i < sizeof(words)/sizeof(words[0]); i++) {
        // dprintf和fprintf类似，只不过第一个参数是文件描述符
        dprintf(sp.supply_fd, "%s", words[i]); // 往文件描述符中写入数据，等价于write
    }
    close(sp.supply_fd);
    // 父进程
    int status;
    pid_t pid = waitpid(sp.pid, &status, 0);
    // 没有学习过如何根据进程退出状态，判断信息
    return pid == sp.pid && WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    return 0;
}