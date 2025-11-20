#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define THREADS 10
#define LIMIT 1000000

void* compute_sum(void* arg) {
    long long* result = malloc(sizeof(long long));
    long long sum = 0;

    for (int i = 1; i <= LIMIT; i++) {
        sum += i;
    }

    *result = sum;
    pthread_exit(result);
}
double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}
int main() {
    pthread_t t[THREADS];
    long long* result;

    printf("Starting thread timing...\n");
    double start_parallel = get_time_sec();

    for (int i = 0; i < THREADS; i++) {
        pthread_create(&t[i], NULL, compute_sum, NULL);
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(t[i], (void**)&result);
        free(result);
    }
    double end_parallel = get_time_sec();
    double parallel_time = end_parallel - start_parallel;

    double start_seq = get_time_sec();

    for (int i = 0; i < THREADS; i++) {
        long long sum = 0;
        for (int j = 1; j <= LIMIT; j++) {
            sum += j;
        }
    }

    double end_seq = get_time_sec();
    double sequential_time = end_seq - start_seq;
    double speedup = sequential_time / parallel_time;

    printf("\n--- RESULTS ---\n");
    printf("Parallel time   : %.6f sec\n", parallel_time);
    printf("Sequential time : %.6f sec\n", sequential_time);
    printf("Speedup         : %.3f×\n", speedup);

    return 0;
}
