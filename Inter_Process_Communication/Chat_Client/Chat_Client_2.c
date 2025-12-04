#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

int main() {
    mkfifo("fifo1", 0666);
    mkfifo("fifo2", 0666);

    pid_t pid = fork();

    if (pid == 0) {
        int fd = open("fifo1", O_RDONLY);
        char msg[100];
        while (1) {
            int n = read(fd, msg, sizeof(msg));
            if (n > 0) {
                msg[n] = '\0';
                if (strcmp(msg, "exit") == 0) break;
                time_t t = time(NULL);
                printf("[Client1 %s] %s\n", ctime(&t), msg);
            }
        }
        close(fd);
    } else {
        int fd = open("fifo2", O_WRONLY);
        char msg[100];
        while (1) {
            fgets(msg, sizeof(msg), stdin);
            msg[strcspn(msg, "\n")] = 0;
            write(fd, msg, strlen(msg) + 1);
            if (strcmp(msg, "exit") == 0) break;
        }
        close(fd);
    }
    return 0;
}
