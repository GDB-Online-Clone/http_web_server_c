#include <stdio.h>
#include <signal.h>

#include "service.h"

#define MAX_PROCESS 16
#define BUF_SIZE 1024

extern struct process_running PROCESSES[MAX_PROCESS];

int main() {
    int temp = 0;
    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < MAX_PROCESS; i++) {
        *PROCESSES[i].to_child_pipe = (int[2]){-1, -1};
        *PROCESSES[i].from_child_pipe = (int[2]){-1, -1};
        PROCESSES[i].pid = 0;
        PROCESSES[i].is_running = 0;
    }

    while (1) {
        printf("1. new process\n2. process list\n3. pass input\n4. check "
               "output\n> ");
        int order;
        int pidx;
        scanf("%d", &order);

        if (order == 1) {
            char path_to_source_code[128];        

            printf("source code path = ");
            scanf("%s", path_to_source_code);
            
            if (build_and_run(path_to_source_code, gcc_c, NULL, NULL) >= 0) {
                printf("SUCCESSFULLY RUN\n");
            }
        } else if (order == 2) {
            show_process_list();
        } else if (order == 3) {
            printf("PROCESS = ");
            scanf("%d", &pidx);

            char buf[1024];
            char *n;
            printf("Enter text (type 'exit' to quit):\n");
            while ((n = fgets(buf, sizeof(buf), stdin)) != NULL) {
                // 'exit'가 입력되면 종료
                if (strncmp(buf, "exit", 4) == 0) {
                    break;
                }
                if (pass_input_to_child(pidx) < 0) {
                    printf("PROCESS CANNOT GET INPUT\n");
                    break;
                }
            }
        } else if (order == 4) {
            printf("PROCESS = ");
            scanf("%d", &pidx);
            char *buf = get_output_from_child(pidx);
            if (buf > 0 && buf != -1) {
                printf("%s", buf);
                free(buf);
            }
        }
    }

    return 0;
}