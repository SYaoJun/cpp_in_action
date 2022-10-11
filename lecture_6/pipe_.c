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
// assignment 3
// sort
// cat unsort.txt | sort 把cat的输出管道，当做sort的读入
int main(int argc, char *argv[]) {
    int fds[2]; pipe(fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(fds[1]);
        char buffer[6];
        read(fds[0],buffer, sizeof(buffer));
        printf("Read from pipe bridging processes: %s.\n", buffer);
        close(fds[0]);
        return 0;
    }
    close(fds[0]);
    write(fds[1], "hello", 6); waitpid(pid, NULL, 0); close(fds[1]); return 0; }