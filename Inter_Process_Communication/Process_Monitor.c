#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUF_SIZE 128
volatile sig_atomic_t change_interval = 0;
volatile sig_atomic_t immediate_update = 0;
volatile sig_atomic_t stop_monitor = 0;

// Signal handlers in child
void sigusr1_handler(int sig) {
    change_interval = 1;
}

void sigusr2_handler(int sig) {
    immediate_update = 1;
}

void sigterm_handler(int sig) {
    stop_monitor = 1;
}

// Function to get timestamp string
void timestamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main() {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // -------------------------
        // Child process
        // -------------------------
        close(fd[0]); // close read end
        signal(SIGUSR1, sigusr1_handler);
        signal(SIGUSR2, sigusr2_handler);
        signal(SIGTERM, sigterm_handler);

        int interval = 5; // default 5 seconds

        while (!stop_monitor) {
            // Handle change interval
            if (change_interval) {
                printf("\nChild: Enter new monitoring interval in seconds: ");
                fflush(stdout);
                scanf("%d", &interval);
                change_interval = 0;
            }

            // Read /proc/loadavg
            FILE *fp = fopen("/proc/loadavg", "r");
            if (!fp) {
                perror("fopen /proc/loadavg");
                break;
            }
            char load[BUF_SIZE];
            fgets(load, sizeof(load), fp);
            fclose(fp);

            // Send to parent
            write(fd[1], load, strlen(load)+1);

            // Wait for interval, check for immediate update every second
            for (int i=0; i<interval; i++) {
                if (stop_monitor) break;
                if (immediate_update) {
                    immediate_update = 0;
                    break;
                }
                sleep(1);
            }
        }

        close(fd[1]);
        exit(0);
    }

    // -------------------------
    // Parent process
    // -------------------------
    close(fd[1]); // close write end
    FILE *logf = fopen("monitor.log", "a");
    if (!logf) {
        perror("fopen log file");
        kill(pid, SIGTERM);
        wait(NULL);
        exit(1);
    }

    char buf[BUF_SIZE];
    while (1) {
        int n = read(fd[0], buf, BUF_SIZE);
        if (n <= 0) break;

        char ts[64];
        timestamp(ts, sizeof(ts));

        // Display formatted stats
        printf("[%s] Load average: %s", ts, buf);

        // Log to file
        fprintf(logf, "[%s] Load average: %s", ts, buf);
        fflush(logf);
    }

    fclose(logf);
    close(fd[0]);

    // Wait for child to finish
    wait(NULL);
    printf("\nMonitoring stopped.\n");

    return 0;
}
