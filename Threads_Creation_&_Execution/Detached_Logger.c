#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

void* logger(void* arg) {
    FILE* f = fopen("log.txt", "a");
    time_t now = time(NULL);
    fprintf(f, "Thread %lu logged at %s", pthread_self(), ctime(&now));
    fclose(f);
    return NULL;
}

int main() {
    pthread_t t[5];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < 5; i++) pthread_create(&t[i], &attr, logger, NULL);

    pthread_attr_destroy(&attr);

    sleep(3);

    int r = pthread_join(t[0], NULL);
    printf("Join attempt result: %d (22 means invalid because the thread is detached)\n", r);

    printf("Main finished\n");
    return 0;
}
