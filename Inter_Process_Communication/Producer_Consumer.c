#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {
    int fd[2];  // pipe: fd[0] = read, fd[1] = write

    if (pipe(fd) == -1) {
        perror("Pipe failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    /* =======================
       PRODUCER PROCESS
       ======================= */
    if (pid == 0) {
        close(fd[0]);  // close read end

        srand(time(NULL));

        for (int i = 1; i <= 20; i++) {
            int num = (rand() % 100) + 1;
            write(fd[1], &num, sizeof(int));
            printf("Producer sent: %d\n", num);
            sleep(1);
        }


        close(fd[1]);  // signal EOF
        exit(0);
    }

    /* =======================
       CONSUMER PROCESS
       ======================= */
    else {
        close(fd[1]);  // close write end

        int num;
        int count = 0;
        double sum = 0;

        while (read(fd[0], &num, sizeof(int)) > 0) {
            count++;
            sum += num;

            double avg = sum / count;
            printf("Consumer received: %d   Running Avg: %.2f\n", num, avg);
        }

        printf("Producer finished. Consumer exiting.\n");

        close(fd[0]);
    }

    return 0;
}
