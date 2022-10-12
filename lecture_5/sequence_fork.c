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
#include <assert.h>
#define N 5
int main() {
    int children[N];
    for(int i = 0; i < N; i++) {
        if((children[i] = fork()) == 0) exit(110+i);
    }

    for(int i = 0; i < N; i++) {
        int status;
        pid_t pid = waitpid(children[i], &status, 0);
        assert(pid == children[i]);
        assert(WIFEXITED(status) && (WEXITSTATUS(status) == (110+i)));
        printf("child = %d, exit status = %d\n", i,WEXITSTATUS(status));
    }
}