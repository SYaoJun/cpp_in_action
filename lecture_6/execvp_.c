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
static int my_system(const char *command) {
    pid_t pid = fork();
    if (pid == 0) {
        char *arguments[] = {"/bin/sh", "-c", (char *) command, NULL};
        execvp(arguments[0], arguments);
        printf("Failed to invoke /bin/sh to execute the supplied command.");
        exit(0);
    }
    // 父进程
    int status;
    waitpid(pid, &status, 0);
    // 没有学习过如何根据进程退出状态，判断信息
    return WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
}

static const size_t kMaxLine = 2048;
int main(int argc, char *argv[]) {
    char command[kMaxLine];
    while (1) {
        printf("> ");
        // 从指定的流读入一行，读到换行符，或者文件末尾，或者n-1个字符时停止，没读到数据返回空指针
        fgets(command, kMaxLine, stdin);
        // 测试是否是文件模式
        if (feof(stdin)) break;
        command[strlen(command) - 1] = '\0'; // overwrite '\n'
        printf("return code = %d\n", my_system(command));
    }
    printf("\n");
    return 0;
}