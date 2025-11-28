#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>

int NUM_PRODUCERS = 2;
int NUM_CONSUMERS = 2;
int MAX_BUFFER = 50;
int PRODUCE_DELAY_US = 20000;
int CONSUME_DELAY_US = 30000;
int RUN_DURATION = 5;
int HIGH_PERCENT = 40;
int FIFO_MODE = 0;

typedef struct {
    int id;
    int priority;
    struct timespec produced_at;
} item_t;

item_t *high_buf;
item_t *low_buf;

int high_head = 0, high_tail = 0, high_count = 0;
int low_head = 0, low_tail = 0, low_count = 0;
int total_count = 0;

uint64_t total_produced = 0;
uint64_t total_consumed = 0;
long double sum_wait_ns = 0.0L;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;

volatile int shutdown_flag = 0;

static inline uint64_t ns(const struct timespec *t) {
    return (uint64_t)t->tv_sec * 1000000000ULL + t->tv_nsec;
}

static inline uint64_t diff_ns(const struct timespec *a, const struct timespec *b) {
    uint64_t an = ns(a), bn = ns(b);
    return (an > bn) ? (an - bn) : 0;
}

void push_item(item_t x) {
    if (FIFO_MODE) {
        low_buf[low_tail] = x;
        low_tail = (low_tail + 1) % MAX_BUFFER;
        low_count++;
        total_count++;
        return;
    }
    if (x.priority == 1) {
        high_buf[high_tail] = x;
        high_tail = (high_tail + 1) % MAX_BUFFER;
        high_count++;
    } else {
        low_buf[low_tail] = x;
        low_tail = (low_tail + 1) % MAX_BUFFER;
        low_count++;
    }
    total_count++;
}

int pop_item(item_t *out) {
    if (total_count == 0) return 0;
    if (FIFO_MODE) {
        *out = low_buf[low_head];
        low_head = (low_head + 1) % MAX_BUFFER;
        low_count--;
        total_count--;
        return 1;
    }
    if (high_count > 0) {
        *out = high_buf[high_head];
        high_head = (high_head + 1) % MAX_BUFFER;
        high_count--;
        total_count--;
        return 1;
    }
    if (low_count > 0) {
        *out = low_buf[low_head];
        low_head = (low_head + 1) % MAX_BUFFER;
        low_count--;
        total_count--;
        return 1;
    }
    return 0;
}

void *producer_fn(void *arg) {
    int id = (int)(intptr_t)arg;
    unsigned int seed = time(NULL) ^ id;
    while (1) {
        pthread_mutex_lock(&mutex);
        if (shutdown_flag) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        while (!shutdown_flag && total_count == MAX_BUFFER)
            pthread_cond_wait(&cond_not_full, &mutex);
        if (shutdown_flag) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        item_t it;
        it.id = ++total_produced;
        it.priority = (rand_r(&seed) % 100 < HIGH_PERCENT);
        clock_gettime(CLOCK_MONOTONIC, &it.produced_at);
        push_item(it);
        pthread_cond_signal(&cond_not_empty);
        pthread_mutex_unlock(&mutex);
        usleep(PRODUCE_DELAY_US);
    }
    return NULL;
}

void *consumer_fn(void *arg) {
    int id = (int)(intptr_t)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        while (!shutdown_flag && total_count == 0)
            pthread_cond_wait(&cond_not_empty, &mutex);
        if (shutdown_flag && total_count == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        item_t it;
        int ok = pop_item(&it);
        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&mutex);

        if (!ok) continue;

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t wait_ns = diff_ns(&now, &it.produced_at);

        __sync_fetch_and_add(&total_consumed, 1);
        pthread_mutex_lock(&mutex);
        sum_wait_ns += wait_ns;
        pthread_mutex_unlock(&mutex);

        usleep(CONSUME_DELAY_US);
    }
    return NULL;
}

void start_shutdown() {
    pthread_mutex_lock(&mutex);
    shutdown_flag = 1;
    pthread_cond_broadcast(&cond_not_empty);
    pthread_cond_broadcast(&cond_not_full);
    pthread_mutex_unlock(&mutex);
}

int main() {
    high_buf = calloc(MAX_BUFFER, sizeof(item_t));
    low_buf = calloc(MAX_BUFFER, sizeof(item_t));

    pthread_t prod[NUM_PRODUCERS], cons[NUM_CONSUMERS];

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_create(&prod[i], NULL, producer_fn, (void*)(intptr_t)i);
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_create(&cons[i], NULL, consumer_fn, (void*)(intptr_t)i);

    sleep(RUN_DURATION);
    start_shutdown();

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(prod[i], NULL);
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond_not_empty);
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(cons[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t2);
    double elapsed = (double)diff_ns(&t2, &t1) / 1e9;

    printf("Produced: %" PRIu64 "\n", total_produced);
    printf("Consumed: %" PRIu64 "\n", total_consumed);
    printf("Throughput: %.2f items/sec\n", total_consumed / elapsed);
    if (total_consumed > 0)
        printf("Average wait: %.3f ms\n", (double)(sum_wait_ns / total_consumed) / 1e6);

    free(high_buf);
    free(low_buf);

    return 0;
}
