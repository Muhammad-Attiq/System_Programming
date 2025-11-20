#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* compute_factorial(void* arg) {
    int n = *(int*)arg;
    long long* result = malloc(sizeof(long long));
    long long fact = 1;
    for (int i = 1; i <= n; i++) {
        fact *= i;
    }
    *result = fact;
    pthread_exit((void*)result);
}

int main() {
    pthread_t threads[5];
    int nums[5];
    long long* result;
    for (int i = 0; i < 5; i++) {
        nums[i] = i + 1;
        pthread_create(&threads[i], NULL, compute_factorial, &nums[i]);
    }
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], (void**)&result);
        printf("Factorial of %d = %lld\n", i + 1, *result);
        free(result);
    }
    printf("All threads completed.\n");
    return 0;
}
