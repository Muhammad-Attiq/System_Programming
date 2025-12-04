#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

volatile int remaining_time = 0;
volatile int paused = 0;

void alarm_handler(int sig) {
    if (!paused && remaining_time > 0) {
        remaining_time--;
        printf("\rTime remaining: %d seconds  ", remaining_time);
        fflush(stdout);
        if (remaining_time == 0) {
            printf("\nTime's up! \a\n");
        } else {
            alarm(1); // schedule next tick
        }
    }
}

void pause_handler(int sig) {
    paused = 1;
    printf("\nTimer paused at %d seconds\n", remaining_time);
}

void resume_handler(int sig) {
    if (paused) {
        paused = 0;
        printf("Timer resumed\n");
        alarm(1); // resume ticking
    }
}

void reset_handler(int sig) {
    remaining_time = 0;
    paused = 0;
    printf("\nTimer reset!\n");
}

int main() {
    signal(SIGALRM, alarm_handler);
    signal(SIGUSR1, pause_handler);
    signal(SIGUSR2, resume_handler);
    signal(SIGINT, reset_handler);

    while (1) {
        printf("Enter countdown time in seconds (0 to exit): ");
        int t;
        if (scanf("%d", &t) != 1) break;
        if (t <= 0) break;

        remaining_time = t;
        paused = 0;

        printf("Timer started for %d seconds\n", remaining_time);
        alarm(1); // start countdown

        // wait until timer finishes or is reset
        while (remaining_time > 0) {
            pause(); // wait for signals
        }

        printf("Timer finished.\n\n");
    }

    printf("Exiting timer app.\n");
    return 0;
}
