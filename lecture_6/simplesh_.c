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
static int kMaxCommandLength = 1024;
static int kMaxArgumentCount = 5;
int main(int argc, char *argv[]) {
    while (1) {
        char command[kMaxCommandLength + 1];
//        readCommand(command, kMaxCommandLength);
        fgets(command, kMaxCommandLength, stdin);
        char *arguments[kMaxArgumentCount + 1];
//        int count = parseCommandLine(command, arguments, kMaxArgumentCount);
        int count = argc;
        if (count == 0) continue;
        if (strcmp(arguments[0], "quit") == 0) break; // hardcoded builtin to exit shell
        int isbg = strcmp(arguments[count - 1], "&") == 0;
        if (isbg) arguments[--count] = NULL; // overwrite "&"
        pid_t pid = fork();
        if (pid == 0) execvp(arguments[0], arguments);
        if (isbg) { // background process, don't wait for child to finish
            printf("%d %s\n", pid, command);
        } else { // otherwise block until child process is complete
            waitpid(pid, NULL, 0);
        }
    }
    printf("\n");
    return 0; }