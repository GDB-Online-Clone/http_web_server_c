#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include <webserver/utility.h>
#include "service.h"

#define MAX_PROCESS 16
#define BUF_SIZE 1024

extern char **environ;

/**
 * @brief The number of created processes
 */
static atomic_int pcnt = 0;

/**
 * @brief List of processes for managing user's run request
 */
struct process_running PROCESSES[MAX_PROCESS];

static int set_FD_CLOEXEC(int fd) {
    // FD_CLOEXEC 플래그 설정
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        perror("fcntl - F_GETFD");
        close(fd);
        return -1;
    }

    flags |= FD_CLOEXEC;

    if (fcntl(fd, F_SETFD, flags) == -1) {
        perror("fcntl - F_SETFD");
        close(fd);
        return -1;
    }
}

int build_and_run(const char *path_to_source_code, enum compiler_type compiler_type, const char *compile_options, const char *command_line_args) {
    int pidx = 0;        
    char executable_filename[64];
    sprintf(executable_filename, "bins/%d.out", pcnt++);    

    int compile_option_cnt = 0;
    char *c_options = NULL;
    char *compile_options_ptr = NULL;

    /* parse compile_options */
    if (compile_options && compile_options[0] != '\0') {
        c_options = strdup(compile_options);
        compile_options_ptr = c_options;
        while ((compile_options_ptr = strstr(compile_options_ptr, " "))) {
            compile_option_cnt++;
            compile_options_ptr++;
        }
    }

    /* actual compiler arguments to be passed with `posix_spawn` */
    char **compile_args = (char **)malloc((compile_option_cnt +  5) * sizeof(char *));
    compile_args[0] = get_compiler_path(compiler_type);
    compile_args[1] = path_to_source_code;
    compile_args[2] = "-o";
    compile_args[3] = executable_filename;
    compile_options_ptr = c_options;
    for (int i = 4; i < 4 + compile_option_cnt; i++) {
        compile_args[i] = compile_options_ptr;
        compile_options_ptr = strstr(compile_options_ptr, " ");
        *compile_options_ptr = '\0';
        compile_options_ptr++;
    }
    compile_args[4 + compile_option_cnt] = (char *)NULL;


    // @TODO PROCESSES need to be read with atomic operation
    for (pidx = 0; pidx < MAX_PROCESS && PROCESSES[pidx].is_running; pidx++)
        ;
    if (pidx == MAX_PROCESS) {
        printf("NO MORE PRECESS\n");
        goto build_and_run_error;
    }

    PROCESSES[pidx] = (struct process_running){
        .is_running = 1,
    };

    /* 파이프 생성 */
    if (pipe(PROCESSES[pidx].from_child_pipe) == -1) {
        perror("pipe");
        goto build_and_run_error;
    }
    if (pipe(PROCESSES[pidx].to_child_pipe) == -1) {
        perror("pipe");
        goto build_and_run_error;
    }

    PROCESSES[pidx].pid = fork();
    if (PROCESSES[pidx].pid == -1) {
        perror("fork");
        goto build_and_run_error;
    } else if (PROCESSES[pidx].pid == 0) {
        /* forked */
        /* 표준 입출력의 리다이렉션 */
        close(PROCESSES[pidx].from_child_pipe[0]);
        dup2(PROCESSES[pidx].from_child_pipe[1], STDOUT_FILENO);
        dup2(PROCESSES[pidx].from_child_pipe[1], STDERR_FILENO);
        close(PROCESSES[pidx].from_child_pipe[1]);

        close(PROCESSES[pidx].to_child_pipe[1]);
        dup2(PROCESSES[pidx].to_child_pipe[0], STDIN_FILENO);
        close(PROCESSES[pidx].to_child_pipe[0]);

        /* 목표 프로세스 실행 */        
        pid_t compiler_pid;
        int compiler_status;        
        for (int i = 0; compile_args[i]; i++) {
            printf("%s ", compile_args[i]);
        }
        printf("\n");
        if (posix_spawn(&compiler_pid, compile_args[0], NULL, NULL, compile_args, environ)) {
            perror("posix_spawn");
        }
        waitpid(compiler_pid, &compiler_status, 0);
        if (!WIFEXITED(compiler_status)) {
          printf("Compile failed (unknown reason) \n");
          exit(EXIT_FAILURE);
        }         
        if (WEXITSTATUS(compiler_status) != 0) {
          printf("Compile failed (compile error) \n");
          exit(EXIT_FAILURE);
        }

        char *cl_args_str = NULL;
        char *cl_args_ptr = NULL;
        int cl_arg_cnt = 0;

        /* parse compile_options */
        if (command_line_args && command_line_args[0] != '\0') {
            cl_args_str = strdup(command_line_args);
            cl_args_ptr = cl_args_str;
            while ((cl_args_ptr = strstr(cl_args_ptr, " "))) {
                cl_arg_cnt++;
                cl_args_ptr++;
            }
        }

        /* actual compiler arguments to be passed with `posix_spawn` */
        char **cl_args = (char **)malloc((cl_arg_cnt +  2) * sizeof(char *));
        cl_args[0] = executable_filename;
        cl_args_ptr = cl_args_str;
        for (int i = 1; i < 1 + cl_arg_cnt; i++) {
            cl_args[i] = cl_args_ptr;
            cl_args_ptr = strstr(cl_args_ptr, " ");
            *cl_args_ptr = '\0';
            cl_args_ptr++;
        }
        cl_args[1 + cl_arg_cnt] = (char *)NULL;
        execv(cl_args[0], cl_args);
        
        /* execv failed */
        perror("exec");
        exit(EXIT_FAILURE);
    }
    close(PROCESSES[pidx].to_child_pipe[0]);
    close(PROCESSES[pidx].from_child_pipe[1]);
    PROCESSES[pidx].to_child_pipe[0] = -1;
    PROCESSES[pidx].from_child_pipe[1] = -1;

    /* 다른 자식 프로세스에서는 해당 파일 기술자 에 접근하지 못하게 하기 위함 */
    set_FD_CLOEXEC(PROCESSES[pidx].to_child_pipe[1]);
    set_FD_CLOEXEC(PROCESSES[pidx].from_child_pipe[0]);

    int inpipe_flags = fcntl(PROCESSES[pidx].from_child_pipe[0], F_GETFL);
    if (fcntl(PROCESSES[pidx].from_child_pipe[0], F_SETFL, inpipe_flags | O_NONBLOCK) ==
        -1) {
        perror("fcntl");
    }

    if (c_options) {
        free(c_options);
    }
    if (compile_args) {
        free(compile_args);
    }
    return pidx;

    build_and_run_error:
    if (c_options) {
        free(c_options);
    }
    if (compile_args) {
        free(compile_args);
    }
    return -1;
}
