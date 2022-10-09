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
#include <csse2310a3.h>
// job

int total_number = 100;
typedef struct Job{
    int pid;
    int num_restarts;
    int verbose;
    char *input;
    char *output;
    char *cmds;
    char **cmd;
    int cmdNum;
    int pipeInput[2];
    int pipeOutput[2];
    int startTimes;
}Job_t;
Job_t* jobs[1024];
typedef struct Param {
    int verbose_mode; // 0 - off, 1 - on
    char* input_file;
    char* job_file;
} Param_t;

typedef struct Fd {
    int input_fd;
    int output_fd;
    int input_pipe[2];
    int output_pipe[2];
    int un_runnable;
} Fd_t;

int checkValidLine(char *line) {
    int k = 0;
    for(int i = 0; i < strlen(line); i++) {
        if(line[i] == ':') {
            k += 1;
        }
    }
    if(k == 3 && line[0] != ':' && line[strlen(line) - 1] != ':') return 1;
    return 0;
}

int checkFileRead(char *fileName) {
    if(fileName == NULL) return 1;
    FILE *fp = fopen(fileName, "r");
    if(fp == NULL) {
        fprintf(stderr, "Error: unable to open \"%s\" for reading\n", fileName);
        return 0;
    }
    fclose(fp);
    return 1;
}

int checkFileWrite(char *fileName) {
    if(fileName == NULL) return 1;
    int fd = open(fileName, (O_CREAT | O_TRUNC) & (S_IWUSR | S_IRUSR));
    if(fd == -1) {
        fprintf(stderr, "Error: unable to open \"%s\" for writing\n", fileName);
        return 0;
    }
    close(fd);
    return 1;
}
//char** split_line(char* line, char c, int *num) {
//    char *ret = NULL;
//    char *temp[1024];
//    int k = 0;
//    char *token = strtok(line, ":");
//    while(token != NULL)
//    {
//        ret = token;
//        temp[k++] = ret;
//        token = strtok(NULL, ":");
//    }
//    *num = k;
//    return temp;
//}
//char** split_space_not_quote(char* line, int* num) {
//    char *ret = NULL;
//    char *temp[1024];
//    int k = 0;
//    char *token = strtok(line, " ");
//    while(token != NULL)
//    {
//        ret = token;
//        temp[k++] = ret;
//        token = strtok(NULL, " ");
//    }
//    *num = k;
//    printf("k = %d\n", k);
//    return temp;
//}
int parseJobLine( Param_t * param, Job_t *job, char *line, int jobsNum) {
    int u;
    char **data = split_line(line, ':');
    job->num_restarts = atoi(strdup(data[0]));
    job->input = strdup(data[1]);
    job->output = strdup(data[2]);
    job->cmds = strdup(data[3]);
    job->cmd = split_space_not_quote(data[3], &job->cmdNum);
    job->verbose = param->verbose_mode;
    job->pid = jobsNum;
    if(!checkFileRead(job->input)) return 0;
    if(!checkFileWrite(job->output)) return 0;
    if(pipe(job->pipeInput) == -1) return 0;
    if(pipe(job->pipeOutput) == -1) return 0;
    if(job->verbose) {
        printf("Registering worker %d: %s", jobsNum, job->cmds);
    }
    return 1;
}



Param_t check_argument(int argc, const char* argv[]) {
    Param_t temp;
    int is_v_exist = 0;
    int is_i_exist = 0;
    // 1. 判重复
    for(int i = 1; i < argc; i ++) {
        // 遍历每个参数
        // 是v参
        if(strcmp(argv[i], "-v") == 0) {
            temp.verbose_mode = 1;
            if(is_v_exist == 1) {
                fprintf(stderr, "Usage: jobthing [-v] [-i inputfile] jobfile");
                exit(1);
            }
            is_v_exist = 1;
            continue;
        }
        // 是i参
        if(strcmp(argv[i], "-i") == 0) {
            if(is_i_exist == 1) {
                fprintf(stderr, "Usage: jobthing [-v] [-i inputfile] jobfile");
                exit(1);
            }
            is_i_exist = 1;
            if((i + 1) >= argc) {
                exit(1);
            }
            const char *inputFile = argv[i+1];
            FILE *fp = fopen(inputFile, "r");
            if(fp == NULL) {
                fprintf(stderr, "Error: Unable to read input file");
                exit(3);
            }
            fclose(fp);
            temp.input_file = strdup(inputFile);
            i += 1;
            continue;
        }
        // 是job file
        FILE *fp = fopen(argv[i], "r");
        if(fp == NULL) {
            fprintf(stderr, "Error: Unable to read job file");
            exit(2);
        }
        fclose(fp);
        temp.job_file = strdup(argv[i]);
    }
    if(temp.job_file == NULL) {
        fprintf(stderr, "Usage: jobthing [-v] [-i inputfile] jobfile");
        exit(1);
    }
    return temp;
}

int is_all_digit(char * s) {
    int len = strlen(s);
    for(int i = 0; i < len; i++) {
        if(!isdigit(s[i])) {
            return 0;
        }
    }
    return 1;
}
int parse_command(char* s) {
    if(strcmp(s, "") == 0) {
        return 0;
    }
    if(s[0]=='*') {
        char *ret = NULL;
        char *temp[1024];
        int k = 0;
        char *token = strtok(s, " ");
        while(token != NULL)
        {
            ret = token;
            temp[k++] = ret;
            token = strtok(NULL, " ");
        }
        if(strcmp(temp[0], "*signal") == 0) {
            if(k != 3) {
                fprintf(stdout, "Error: Incorrect number of arguments");
                exit(1);
            }
            if(is_all_digit(temp[1]) == 0) {
                fprintf(stdout, "Error: Invalid job");
                exit(1);
            }else{
                int job_id = strtol(temp[1], NULL, 10);
                if(job_id < 1 || job_id > total_number ) {
                    fprintf(stdout, "Error: Invalid job");
                    exit(1);
                }
                if(is_all_digit(temp[2]) == 0) {
                    fprintf(stdout, "Error: Invalid job");
                    exit(1);
                }else{
                    int signal_id = strtol(temp[2], NULL, 10);
                    if(signal_id < 1 || signal_id > 31 ) {
                        fprintf(stdout, "Error: Invalid signal");
                        exit(1);
                    }
                    // 执行命令
                    pid_t  pid = jobs[job_id];
                    kill(pid, signal_id);
                    return 0;
                }
            }
        }else if(strcmp(temp[0], "*sleep") == 0){
            if(k != 2) {
                fprintf(stdout, "Error: Incorrect number of arguments");
                exit(1);
            }
            if(is_all_digit(temp[1]) == 0) {
                fprintf(stdout, "Error: Incorrect number of arguments");
                exit(1);
            }else{
                int duration = strtol(temp[1], NULL, 10);
                if(duration < 0) {
                    fprintf(stdout, "Error: Invalid duration");
                    exit(1);
                }
                // 执行命令
                usleep(duration);
                return 0;
            }
        }else{
            fprintf(stdout, "Error: Bad command 'cmd'");
            exit(1);
        }
    }

    return 1;
}

void job_startup_phase(Job_t* job, int job_number) {
    Fd_t *temp = (Fd_t*) malloc(sizeof(Fd_t));
    temp->input_fd = -1;
    temp->output_fd = -1;

    if(strcmp(job->input, "") == 0) {
        pipe(temp->input_pipe);
    }else{
        int fd = open(job->input, O_RDONLY);
        // open失败返回-1
        if(fd == -1) {
            fprintf(stderr, "Error: unable to open \"%s\" for reading", job->input);
            temp->un_runnable = 1;
            exit(3);
        }
        temp->input_fd = fd;
    }

    if(strcmp(job->output, "") == 0) {
        pipe(temp->output_pipe);
    }else{
        int fd_out = open(job->output, O_WRONLY);
        if(fd_out == -1) {
            fprintf(stderr, "Error: unable to open \"%s\" for writing", job->output);
            temp->un_runnable = 1;
            exit(3);
        }
        temp->output_fd = fd_out;
    }

    pid_t pid = fork();
    if(job->verbose) {
        fprintf(stdout, "Spawning worker %d", job_number);
    }

    if(pid < 0) {
        printf("fork error\n");
        exit(0);
    }else if(pid == 0) {
        // 子进程 input
        if(temp->input_fd != -1) {
            // 走文件
            dup2(STDIN_FILENO, temp->input_fd);
            close(temp->input_fd);
        }else{
            // 走管道
            close(temp->input_pipe[1]);
        }

        // output
        if(temp->output_fd != -1) {
            // 走文件
            dup2(STDOUT_FILENO, temp->output_fd);
            close(temp->output_fd);
        }else{
            close(temp->output_pipe[0]);
        }

        printf("child = %d\n", getpid());
        execlp(job->cmd[0],(char* const*)job->cmd, NULL);//执行ls -al
        // 如果成功执行 下面不会运行
        perror("exec failed\n");
        _exit(99);
    }else{
        // input
        if(temp->input_fd != -1) {
            close(temp->input_fd);
        }else{
            close(temp->input_pipe[0]);
        }
        // output
        if(temp->output_fd != -1) {
            close(temp->output_fd);
        }else{
            close(temp->output_pipe[1]);
        }
        char buf[1024];
        read(STDIN_FILENO, buf, sizeof(buf));
        if(parse_command(buf) == 1) {
            write(temp->input_fd, buf, sizeof buf);
            fprintf(stdout, "%d <-'%s'", 1, buf);
        }
    }
}

//// 每行最大长度
//#define LINE_MAX 1024
//
//char** read_line(char *path, int* num)
//{
//    char* temp[1024];
//    int k = 0;
//    FILE *fp;
//    int line_num = 0; // 文件行数
//    int line_len = 0; // 文件每行的长度
//    char buf[LINE_MAX] = {0}; // 行数据缓存
//
//    fp = fopen(path, "r");
//    if (NULL == fp) {
//        printf("open %s failed.\n", path);
//        return -1;
//    }
//
//    while(fgets(buf, LINE_MAX, fp)) {
//        line_num++;
//        line_len = strlen(buf);
//        // 排除换行符
//        if ('\n' == buf[line_len - 1]) {
//            buf[line_len - 1] = '\0';
//            line_len--;
//            if (0 == line_len) {
//                //空行
//                continue;
//            }
//        }
//        // windos文本排除回车符
//        if ('\r' == buf[line_len - 1]) {
//            buf[line_len - 1] = '\0';
//            line_len--;
//            if (0 == line_len) {
//                //空行
//                continue;
//            }
//        }
//        printf("%s\n", buf);
//        temp[k++] = buf;
//    }
//
//    if (0 == feof) {
//        // 未读到文件末尾
//        printf("fgets error\n");
//        return -1;
//    }
//    fclose(fp);
//    *num = k;
//    return temp;
//}
void jobthing_operation(Param_t* param) {
    // 主进程
    // 监测每一个job的状态
    // job_id
    while() {
        int total_num = 0;
        int reclaim_num = 0;
        while(1){
            size_t pid = waitpid(-1, NULL, WNOHANG);
            if (pid > 0) {
                reclaim_num++;
            }
            if(reclaim_num == total_num) break;
        };
    }
}
void sighup_handler() {

}
void read_job_file(){

}
/*
 * 1. jobs数组
 * 2. jobthing读一行文件，发送给每个job，如果是*开头则表示命令
 * 3. jobthing从管道读入数据，直到行结束
 * */
int main(int argc, const char* argv[]) {
    Param_t parameter;
    Job_t* job;
    // 1. 检查参数
    parameter = check_argument(argc, argv);
    puts("argument is ok!");

    // 2. 从配置文件读出每一行
    read_job_file(parameter.job_file);
    puts("read_job_file is ok!");

    int line_num = 0;
    char** lines = read_line(parameter.job_file, &line_num);
    // 有这么多个job
    int current_k = 0;
    for(int i = 0; i < line_num; i++) {
        if(strcmp(lines[i], "\n") == 0) continue;
        if(lines[i][0] == '#') continue;
        if(checkValidLine(lines[i])) {

            if(parseJobLine(&parameter, lines[i], job, current_k)) {
                current_k++;
                // 3. job启动阶段
                job_startup_phase(job, current_k);
                jobs[current_k] = job;
            }
        }
    }
    sleep(1);
    jobthing_operation(parameter);
}