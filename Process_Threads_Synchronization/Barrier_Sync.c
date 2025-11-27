#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_THREADS 50
#define BARRIER_ITERATIONS 1000   // how many times barrier is used
#define WORKLOAD 10000            // work done before each barrier

// ------------------------------------------------------------
//                REUSABLE BARRIER STRUCTURE
// ------------------------------------------------------------
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int count;          // number of threads that reached barrier
    int total;          // total threads that must reach barrier
    int generation;     // to differentiate barrier reuse
} barrier_t;

void barrier_init(barrier_t *b, int total_threads) {
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->total = total_threads;
    b->generation = 0;
}

// Reusable barrier using mutex + condition variable
void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->lock);

    int gen = b->generation;

    b->count++;

    // Last thread to reach the barrier wakes up all others
    if (b->count == b->total) {
        b->generation++;         // move to next generation
        b->count = 0;            // reset for next reuse
        pthread_cond_broadcast(&b->cond);
    } else {
        // Wait until all threads reach the same generation
        while (gen == b->generation)
            pthread_cond_wait(&b->cond, &b->lock);
    }

    pthread_mutex_unlock(&b->lock);
}

// ------------------------------------------------------------
//                     WORKER THREAD FUNCTION
// ------------------------------------------------------------
typedef struct {
    int id;
    barrier_t *barrier;
} thread_arg_t;

void do_work(int id) {
    // Simulate heavy computation
    volatile long x = 0;
    for (int i = 0; i < WORKLOAD; i++) {
        x += (id + i) % 7;
    }
}

void* worker(void *arg) {
    thread_arg_t *t = (thread_arg_t*)arg;

    for (int i = 0; i < BARRIER_ITERATIONS; i++) {

        do_work(t->id);          // do some computation

        barrier_wait(t->barrier);  // wait at barrier

        // Section after barrier
        do_work(t->id);
    }

    return NULL;
}

// ------------------------------------------------------------
//                     TIME MEASUREMENT
// ------------------------------------------------------------
long long timestamp_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec*1000000LL + ts.tv_nsec/1000;
}

// ------------------------------------------------------------
//                          MAIN PROGRAM
// ------------------------------------------------------------
int main(int argc, char *argv[]) {

    int num_threads = 10;  
    if (argc == 2) num_threads = atoi(argv[1]);

    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Invalid number of threads\n");
        return 1;
    }

    printf("Barrier Synchronization Test\n");
    printf("Threads           : %d\n", num_threads);
    printf("Barrier Iterations: %d\n", BARRIER_ITERATIONS);
    printf("Workload per Iter : %d\n\n", WORKLOAD);

    pthread_t threads[num_threads];
    thread_arg_t args[num_threads];
    barrier_t barrier;

    barrier_init(&barrier, num_threads);

    long long parallel_start = timestamp_us();

    // Start threads
    for (int i = 0; i < num_threads; i++) {
        args[i].id = i;
        args[i].barrier = &barrier;
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    long long parallel_end = timestamp_us();

    long long parallel_time = parallel_end - parallel_start;
    printf("\nParallel Execution Time: %lld microseconds\n", parallel_time);

    // ------------------------------------------------------------
    //         SEQUENTIAL EXECUTION FOR PERFORMANCE COMPARISON
    // ------------------------------------------------------------
    long long seq_start = timestamp_us();

    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < BARRIER_ITERATIONS; i++) {
            do_work(t);
            do_work(t);
        }
    }

    long long seq_end = timestamp_us();
    long long seq_time = seq_end - seq_start;

    printf("Sequential Execution Time: %lld microseconds\n", seq_time);
    printf("\nSpeedup: %.2fx\n", (double)seq_time / parallel_time);

    return 0;
}
