
#include <sys/types.h>

/**
 * @brief Information of process which created with user source code
 */
struct process_running {
    pid_t pid;
    int from_child_pipe[2];
    int to_child_pipe[2];
    int is_running;
};

/**
 * @brief types of compilers and langs
 */
enum compiler_type {
    gcc_c,
    gcc_cpp,
    clang_c,
    clang_cpp
};

/**
 * @brief Get the compiler binary filepath as literal string
 * 
 * @param compiler_type type of compiler and lang
 * @return Compiler binary filepath as literal string
 */
static char *get_compiler_path(enum compiler_type compiler_type) {
    if (compiler_type == gcc_c) {
        return "/usr/bin/gcc";
    }
    if (compiler_type == gcc_cpp) {
        return "/usr/bin/g++";
    }
    if (compiler_type == clang_c) {
        return "/usr/bin/clang";
    }
    if (compiler_type == clang_cpp) {
        return "/usr/bin/clang++";
    }
}

/**
 * @brief Build and run one user's processes
 * 
 * @param path_to_source_code Path to source code (Can be both relative path or absoulute path)
 * @param compile_options Additional compiler options. 
 * NULL is considered as nothing. Each options should be seperated by whitespace and trimmed.
 * @param command_line_args Command line arguments for run process. Do not need to pass executable file name. 
 * NULL is considered as nothing. Each options should be seperated by whitespace and trimmed.
 * @return Successfully do fork, at least, return non minus integers that represents for id of process. Or return -1.
 * @retval -1 Failed to reach to step to `fork`.
 * @note Arguments need to have lifetime that is longer than this function.
 */
int build_and_run(const char *path_to_source_code, enum compiler_type compiler_type, const char *compile_options, const char *command_line_args);

/**
 * @brief See processes list for debugging
 */
void show_process_list();

/**
 * @brief Stop the process by given id 
 * 
 * @param pidx id of process that `build_and_run` have returned.
 * @return 1 if success, or 0.
 */
int stop_process(int pidx);

