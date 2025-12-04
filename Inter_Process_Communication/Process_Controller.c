#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

volatile sig_atomic_t paused_flag = 0;
volatile sig_atomic_t counter = 0;

void handle_sigusr1(int sig) {
    paused_flag = 1;
    printf("[Child] Paused at counter = %d\n", counter);
}

void handle_sigusr2(int sig) {
    paused_flag = 0;
    printf("[Child] Resumed\n");
}


void handle_sigint(int sig) {
    counter = 0;
    printf("[Child] Counter reset to 0\n");
}

void handle_sigterm(int sig) {
    printf("[Child] Terminating gracefully...\n");
    exit(0);
}

int main() {
    pid_t pid = fork();

    if (pid < 0) exit(1);

    if (pid == 0) {
        signal(SIGUSR1, handle_sigusr1);
        signal(SIGUSR2, handle_sigusr2);
        signal(SIGINT,  handle_sigint);
        signal(SIGTERM, handle_sigterm);

        while (1) {
            if (!paused_flag) {
                counter++;
                printf("Child Count: %d\n", counter);
                fflush(stdout);
            }
            usleep(300000);
        }
    }

    printf("Commands: pause | resume | reset | exit\n");

    char input[20];

    while (1) {
        printf("Enter command: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            continue;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "pause") == 0) kill(pid, SIGUSR1);
        else if (strcmp(input, "resume") == 0) kill(pid, SIGUSR2);
        else if (strcmp(input, "reset") == 0) kill(pid, SIGINT);
        else if (strcmp(input, "exit") == 0) {
            kill(pid, SIGTERM);
            wait(NULL);
            break;
        }
    }

    return 0;
}
