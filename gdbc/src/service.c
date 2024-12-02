#ifdef __clang__
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

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

static inline int check_pidx(int pidx) {
    if (pidx < 0 || pidx >= MAX_PROCESS || !PROCESSES[pidx].is_running) {
        DLOGV("PID is invalid\n");
        return 0;
    }
    return 1;
}

static int set_FD_CLOEXEC(int fd) {
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        perror("fcntl - F_GETFD");
        close(fd);
        return 0;
    }

    flags |= FD_CLOEXEC;

    if (fcntl(fd, F_SETFD, flags) == -1) {
        perror("fcntl - F_SETFD");
        close(fd);
        return 0;
    }
    return 1;
}

static pid_t check_pid_alive(pid_t pid) {
    printf("CHECK PID %d ... ", pid);
    if (kill(pid, 0) == 0) {
        printf("GOOD\n");
        return pid;
    }
    return 0;
}

static int cleanup_child_process(struct process_running *p_info) {
    if (p_info->is_running == 0) {
        return 0;
    }
    pid_t ret = waitpid(p_info->pid, NULL, WNOHANG);
    if (ret == 0) {
        return 0;
    }
    close(p_info->to_child_pipe[1]);
    close(p_info->from_child_pipe[0]);

    p_info->to_child_pipe[1] = -1;
    p_info->from_child_pipe[0] = -1;

    p_info->is_running = 0;
    return 1;
}

int build_and_run(const char *path_to_source_code, enum compiler_type compiler_type, const char *compile_options, const char *command_line_args, int is_gdb) {
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
    compile_option_cnt += is_gdb;
    char **compile_args = (char **)malloc((compile_option_cnt +  5) * sizeof(char *));
    compile_args[0] = get_compiler_path(compiler_type);
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
    compile_args[1] = path_to_source_code;
#pragma GCC diagnostic pop
    compile_args[2] = "-o";
    compile_args[3] = executable_filename;
    if (is_gdb) {
        compile_args[4] = "-g";
    }
    compile_options_ptr = c_options;
    for (int i = 4 + is_gdb; i < 4 + compile_option_cnt; i++) {
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
        cl_arg_cnt += is_gdb;
        char **cl_args = (char **)malloc((cl_arg_cnt +  2) * sizeof(char *));
        if (is_gdb) {
            cl_args[0] = "/usr/bin/gdb";
        }
        cl_args[0 + is_gdb] = executable_filename;
        cl_args_ptr = cl_args_str;
        for (int i = 1 + is_gdb; i < 1 + cl_arg_cnt; i++) {
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

void show_process_list() {
    for (int i = 0; i < MAX_PROCESS; i++) {        
        printf("**[%5d] PID: %7d\tSTATUS: %s\n", i, PROCESSES[i].pid,
               PROCESSES[i].is_running ? "\033[32mACTIVE\033[0m"
                                : "\033[31mINACTIVE\033[0m");
    }
}

int stop_process(int pidx) {
    if (check_pidx(pidx) == 0)
        return 0;
    if (kill(PROCESSES[pidx].pid, SIGKILL) == -1) {
        perror("kill");        
        return 0;
    }
    return 1;
}

int pass_input_to_child(int pidx, char *input) {
    if (check_pidx(pidx) == 0)
        return -2;
    if (!check_pid_alive(PROCESSES[pidx].pid)) {
        printf("CANNOT ACCESS TO PROCESS\n");
        return -1;
    }

    ssize_t n;
    n = write(PROCESSES[pidx].to_child_pipe[1], input, strlen(input));
    if (n < 0) {
        printf("PROCESS CANNOT GET INPUT\n");
        return -1;
    }
    return n;
}

char *get_output_from_child(int pidx) {
    if (check_pidx(pidx) == 0)
        return -2;
    int available_bytes;
    if (ioctl(PROCESSES[pidx].from_child_pipe[0], FIONREAD, &available_bytes) == -1) {
        perror("ioctl");        
        return (char *)-1;        
    }
        
    char *buf = malloc(available_bytes + 1);    
    ssize_t n;
    if ((n = read(PROCESSES[pidx].from_child_pipe[0], buf, available_bytes) <= 0)) {
        free(buf);
        if (n == 0) {
            if (!check_pid_alive(PROCESSES[pidx].pid)) {
                cleanup_child_process(&PROCESSES[pidx]);
                return (char *)-1;
            }
            return NULL;
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {            
            return NULL;
        }
        printf("CANNOT READ ANYMORE (in any reason)\n");
        printf("Check is PROCESS DONE ... ");
        if (cleanup_child_process(&PROCESSES[pidx])) {
            printf("EXITED\n");
            return (char *)-1;
        } else {
            printf("KILL this\n");
            stop_process(pidx);
            cleanup_child_process(&PROCESSES[pidx]);
            return (char *)-1;
        }
    }
    buf[available_bytes] = '\0';
    return buf;
}