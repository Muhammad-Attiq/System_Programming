#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 1000
#define THREADS 4

int arr[SIZE];

void* partial_sum(void* arg) {
    int* range = (int*)arg;
    int start = range[0];
    int end = range[1];

    long long* sum = malloc(sizeof(long long));
    *sum = 0;

    for (int i = start; i <= end; i++) {
        *sum += arr[i];
    }

    pthread_exit(sum);
}

int main() {
    pthread_t threads[THREADS];
    int ranges[THREADS][2];
    long long* partial;
    long long total_sum = 0;

    for (int i = 0; i < SIZE; i++) {
        arr[i] = i + 1;
    }

    int segment = SIZE / THREADS;

    for (int i = 0; i < THREADS; i++) {
        ranges[i][0] = i * segment;
        ranges[i][1] = (i + 1) * segment - 1;
        pthread_create(&threads[i], NULL, partial_sum, ranges[i]);
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], (void**)&partial);
        total_sum += *partial;
        free(partial);
    }

    long long expected = (long long)SIZE * (SIZE + 1) / 2;

    printf("Computed total sum = %lld\n", total_sum);
    printf("Expected total     = %lld\n", expected);

    if (total_sum == expected)
        printf("Result Verified\n");
    else
        printf("Result Incorrect\n");

    return 0;
}
