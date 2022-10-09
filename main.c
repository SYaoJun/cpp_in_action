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
// job

int total_jobs = 0;
typedef struct Job{
    int pid;
    int no;
    int num_restarts;
    int verbose;
    char *input;
    char *output;
    char *cmds;
    char **cmd;
    int cmdNum;
    int pipeInput[2];
    int pipeOutput[2];
    int used_times;
    int input_fd;
    int output_fd;
    int input_pipe[2];
    int output_pipe[2];
    int runnable;
    int valid;
    int terminated_times;
}Job_t;

// 所有的jobs
Job_t* jobs[1024];

typedef struct Param {
    int verbose_mode; // 0 - off, 1 - on
    char* input_file;
    char* job_file;
} Param_t;

char** split_line(char* line, char c, int *num) {
    char *ret = NULL;
    char *temp[1024];
    int k = 0;
    char *token = strtok(line, ":");
    while(token != NULL)
    {
        ret = token;
        temp[k++] = ret;
        token = strtok(NULL, ":");
    }
    *num = k;
    return temp;
}
char** split_space_not_quote(char* line, int* num) {
    char *ret = NULL;
    char *temp[1024];
    int k = 0;
    char *token = strtok(line, " ");
    while(token != NULL)
    {
        ret = token;
        temp[k++] = ret;
        token = strtok(NULL, " ");
    }
    *num = k;
    printf("k = %d\n", k);
    return temp;
}
/// 每行最大长度
#define LINE_MAX 1024

char** read_line(char *path, int* num)
{
    char* temp[1024];
    int k = 0;
    FILE *fp;
    int line_num = 0; // 文件行数
    int line_len = 0; // 文件每行的长度
    char buf[LINE_MAX] = {0}; // 行数据缓存

    fp = fopen(path, "r");
    if (NULL == fp) {
        printf("open %s failed.\n", path);
        return -1;
    }

    while(fgets(buf, LINE_MAX, fp)) {
        line_num++;
        line_len = strlen(buf);
        // 排除换行符
        if ('\n' == buf[line_len - 1]) {
            buf[line_len - 1] = '\0';
            line_len--;
            if (0 == line_len) {
                //空行
                continue;
            }
        }
        // windos文本排除回车符
        if ('\r' == buf[line_len - 1]) {
            buf[line_len - 1] = '\0';
            line_len--;
            if (0 == line_len) {
                //空行
                continue;
            }
        }
        printf("%s\n", buf);
        temp[k++] = buf;
    }

    if (0 == feof) {
        // 未读到文件末尾
        printf("fgets error\n");
        return -1;
    }
    fclose(fp);
    *num = k;
    return temp;
}
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
    char buf[1024];
    int fd = open(fileName, (O_CREAT | O_TRUNC) & (S_IWUSR | S_IRUSR));
    if(fd == -1) {
        fprintf(stderr, "Error: unable to open \"%s\" for writing\n", fileName);
        return 0;
    }
    close(fd);
    return 1;
}

int parseJobLine( Param_t * param, Job_t *job, char *line) {
    int u;
    char **data = split_line(line, ':', &u);
    job->num_restarts = atoi(strdup(data[0]));
    job->input = strdup(data[1]);
    job->output = strdup(data[2]);
    job->cmds = strdup(data[3]);
    job->cmd = split_space_not_quote(data[3], &job->cmdNum);
    job->verbose = param->verbose_mode;
    if(!checkFileRead(job->input)) return 0;
    if(!checkFileWrite(job->output)) return 0;
    if(pipe(job->pipeInput) == -1) return 0;
    if(pipe(job->pipeOutput) == -1) return 0;
    return 1;
}



Param_t check_argument(int argc, const char* argv[]) {
    Param_t temp;
    char *t = "Usage: jobthing [-v] [-i inputfile] jobfile";
    char *tt = "Error: Unable to read input file";
    int is_v_exist = 0;
    int is_i_exist = 0;
    // 1. 判重复
    for(int i = 1; i < argc; i ++) {
        // 遍历每个参数
        // 是v参
        if(strcmp(argv[i], "-v") == 0) {
            temp.verbose_mode = 1;
            if(is_v_exist == 1) {
                write(STDERR_FILENO, t, strlen(t));
                exit(1);
            }
            is_v_exist = 1;
            continue;
        }
        // 是i参
        if(strcmp(argv[i], "-i") == 0) {
            if(is_i_exist == 1) {
                write(STDERR_FILENO, t, strlen(t));
                exit(1);
            }
            is_i_exist = 1;
            if((i + 1) >= argc) {
                exit(1);
            }
            const char *inputFile = argv[i+1];
            FILE *fp = fopen(inputFile, "r");
            if(fp == NULL) {
                write(STDERR_FILENO, tt, strlen(tt));
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
            write(STDERR_FILENO, tt, strlen(tt));
            exit(2);
        }
        fclose(fp);
        temp.job_file = strdup(argv[i]);
    }
    if(temp.job_file == NULL) {
        write(STDERR_FILENO, t, strlen(t));
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
int parse_command(char* s, int *num, int *s_id, int* j_id, int* d) {
        char *ret = NULL;
        char *temp[1024];
        int k = 0;
        char buf[1024];
        char *token = strtok(s, " ");
        while(token != NULL)
        {
            ret = token;
            temp[k++] = ret;
            token = strtok(NULL, " ");
        }
        *num = k;
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
                if(job_id < 1 || job_id > total_jobs ) {
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
                    *j_id = job_id;
                    *s_id = signal_id;

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

                *d = duration;
                return 0;
            }
        }else{
            fprintf(stdout, "Error: Bad command 'cmd'");
            exit(1);
        }
}

void job_startup_phase() {
    for(int i = 0; i < total_jobs; i++) {
        Job_t *temp = jobs[i];
        // 不合法 跳过
        if(temp->valid == 0) continue;
        temp->input_fd = -1;
        temp->output_fd = -1;

        if (strcmp(temp->input, "") == 0) {
            pipe(temp->input_pipe);
        } else {
            int fd = open(temp->input, O_RDONLY);
            // open失败返回-1
            if (fd == -1) {
                fprintf(stderr, "Error: unable to open \"%s\" for reading", temp->input);
                temp->runnable = 0;
                exit(3);
            }
            temp->input_fd = fd;
        }

        if (strcmp(temp->output, "") == 0) {
            pipe(temp->output_pipe);
        } else {
            int fd_out = open(temp->output, O_WRONLY);
            if (fd_out == -1) {
                fprintf(stderr, "Error: unable to open \"%s\" for writing", temp->output);
                temp->runnable = 0;
                exit(3);
            }
            temp->output_fd = fd_out;
        }

        pid_t pid = fork();
        if (temp->verbose) {
            fprintf(stdout, "Spawning worker %d", temp->no);
        }

        if (pid < 0) {
            printf("fork error\n");
            exit(0);
        } else if (pid == 0) {
            // 子进程 input
            temp->pid = getpid();
            temp->used_times = 1;
            if (temp->input_fd != -1) {
                // 走文件
                dup2(temp->input_fd, STDIN_FILENO);
                close(temp->input_fd);
            } else {
                // 走管道
                close(temp->input_pipe[1]);
            }

            // output
            if (temp->output_fd != -1) {
                // 走文件
                dup2(temp->output_fd, STDOUT_FILENO);
                close(temp->output_fd);
            } else {
                close(temp->output_pipe[0]);
            }

            printf("child = %d\n", getpid());
            execlp(temp->cmd[0], (char *const *) temp->cmd, NULL);//执行ls -al
            // 如果成功执行 下面不会运行
            perror("exec failed\n");
            _exit(99);
        } else {
            // input
            if (temp->input_fd != -1) {
                close(temp->input_fd);
            } else {
                close(temp->input_pipe[0]);
            }
            // output
            if (temp->output_fd != -1) {
                close(temp->output_fd);
            } else {
                close(temp->output_pipe[1]);
            }
        }
    }
}

void jobthing_operation(Param_t* param) {
    // 主进程
    // 监测每一个job的状态
    // job_id
    FILE *fp = fopen(param->input_file, "r");
    char cmd[1024];
    int common_cmd = 0;
    int viable_works = 0;

    while(scanf("%s", cmd) != EOF) {
        // 给每个job发送这个命令
        if(strcmp(cmd, "") == 0) continue;
        if(cmd[0] != '*'){
            common_cmd = 1;
        }
        int argc = 0;
        int job_id = 0;
        int signal_id = 0;
        int duration = 0;
        parse_command(cmd, &argc, &signal_id, &job_id, &duration);
        if(argc == 3){
            pid_t pid = jobs[job_id]->pid;
            kill(pid, signal_id);
        }else if(argc == 2){
            usleep(duration);
        }
        for(int i = 0; i < total_jobs; i++) {
            Job_t * job = jobs[i];
            if(job->used_times >= job->num_restarts) {
                continue;
            }else{
                viable_works++;
                pid_t t_pid = fork();
                if (job->verbose) {
                    fprintf(stdout, "Restarting worker %d", job->no);
                }
                if(t_pid  > 0) {
                    // 父进程
                    write(job->input_fd, cmd, sizeof(cmd));
                    fprintf(stdout, "%d <-'%s'", job->no, cmd);
                    int status;
                    size_t pid = waitpid(-1, &status, WNOHANG);
                    if (pid > 0) {
                        job->terminated_times++;
                    }
                    char buf[1024];
                    if(WEXITSTATUS(status)) {
                        memset(buf, 0, sizeof buf);
                        sprintf(buf, "Job %d has terminated with exit code %d", job->no, WEXITSTATUS(status));
                        write(STDOUT_FILENO, buf, strlen(buf));
                    }else if(WIFSIGNALED(status)) {
                        memset(buf, 0, sizeof buf);
                        sprintf(buf, "Job %d has terminated due to signal %d", job->no, WTERMSIG(status));
                        write(STDERR_FILENO, buf, strlen(buf));
                    }

                }else if(t_pid == 0){
                    // 子进程
                    job->pid = getpid();
                    job->used_times++;
                    char t[1024];
                    read(job->output_fd, t, sizeof(t));
                    fprintf(stdout, "%d ->'%s'", job->no, t);
                    kill(job->pid, 9);
                }
            }
        }
        if(viable_works == 0) {
            fprintf(stderr, "No more viable workers, exiting");
            exit(0);
        }
    }
}
void sighup_handler() {

}

/*
 * 1. jobs数组
 * 2. jobthing读一行文件，发送给每个job，如果是*开头则表示命令
 * 3. jobthing从管道读入数据，直到行结束
 * */
int main(int argc, const char* argv[]) {
    Param_t parameter;
    // 1. 检查参数
    parameter = check_argument(argc, argv);
    // 2. 从配置文件读出每一行
    char * line;
    int lines_num = 0;
    char** total_lines = read_line(parameter.job_file, &lines_num);
    int idx = 0;
    while(idx < lines_num) {
        line = total_lines[idx++];
        if(strcmp(line, "\n") == 0) continue;
        if(line[0] == '#') continue;
        Job_t *temp_job = (Job_t*) malloc(sizeof(Job_t));
        total_jobs++;
        temp_job->no = total_jobs;
        if(checkValidLine(line)) {
            if(parseJobLine(&parameter, line, temp_job) == 1) {
                temp_job->valid = 1;
                fprintf(stdout, "Registering worker %d: %s", temp_job->no, temp_job->cmds);
            }else{
                fprintf(stderr, "Error: invalid job specification: %s", line);
                temp_job->valid = 0;
            }
        }else{
            temp_job->valid = 0;
        }
        jobs[total_jobs] = temp_job;
    }
    // 4. jobs启动阶段
    job_startup_phase();
    sleep(1);

    // 5. jobs操作阶段
    jobthing_operation(&parameter);
}