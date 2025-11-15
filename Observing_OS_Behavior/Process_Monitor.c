#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STATUS_PATH_SIZE 64
#define BUFFER_SIZE 256

int main() {
    FILE *fp;
    char path[STATUS_PATH_SIZE];
    char buffer[BUFFER_SIZE];
    int pid;

    printf("Starting a long-lived process: yes > /dev/null &\n");
    system("yes > /dev/null &");
    sleep(1);

    fp = popen("pidof yes", "r");
    if (fp == NULL) {
        perror("popen failed");
        exit(1);
    }
    fscanf(fp, "%d", &pid);
    pclose(fp);

    printf("Monitoring process PID = %d\n", pid);

    for (int i = 0; i < 5; i++) {
        snprintf(path, STATUS_PATH_SIZE, "/proc/%d/status", pid);
        fp = fopen(path, "r");
        if (!fp) {
            printf("Process ended.\n");
            break;
        }

        printf("\n=== Snapshot %d ===\n", i + 1);
        while (fgets(buffer, BUFFER_SIZE, fp)) {
            if ((strncmp(buffer, "Name:", 5) == 0) ||
                (strncmp(buffer, "Pid:", 4) == 0) ||
                (strncmp(buffer, "State:", 6) == 0) ||
                (strncmp(buffer, "VmRSS:", 6) == 0) ||
                (strncmp(buffer, "VmSize:", 7) == 0) ||
                (strncmp(buffer, "voluntary_ctxt_switches:", 24) == 0) ||
                (strncmp(buffer, "nonvoluntary_ctxt_switches:", 27) == 0)) {
                    printf("%s", buffer);
            }
        }
        fclose(fp);
        sleep(2);
    }

    printf("\nKilling the monitored process...\n");
    char kill_cmd[64];
    snprintf(kill_cmd, sizeof(kill_cmd), "kill %d", pid);
    system(kill_cmd);

    return 0;
}
