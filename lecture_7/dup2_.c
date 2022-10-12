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

int main() {
//    int fd = open("ppxp.txt", O_CREAT | O_RDWR, 0744);
//    if(fd == -1) {
//        printf("open failed!\n");
//        exit(1);
//    }
//    dup2(fd, STDOUT_FILENO); // 原来访问STDOUT_FILENO的，现在变成了访问fd
//    // 操作标准输入
//    write(STDOUT_FILENO, "bai ge", 6);
        printf("hhhh\n");
        // 执行dup2可执行文件
//        exec函数（" ./a.out  "）
    return 0;
}