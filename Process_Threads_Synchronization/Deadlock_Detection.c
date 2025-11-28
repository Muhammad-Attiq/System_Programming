#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#define N_THREADS 8
#define M_RESOURCES 5
#define MAX_INSTANCES 3
#define DETECT_INTERVAL_MS 500
#define RUN_SECONDS 15

enum { STRAT_RANDOM = 0, STRAT_LEAST_ALLOC = 1 };

int strategy = STRAT_LEAST_ALLOC;

int available[M_RESOURCES];
int alloc[N_THREADS][M_RESOURCES];
int request_vec[N_THREADS][M_RESOURCES];
int waiting[N_THREADS];
int running = 1;

pthread_mutex_t sys_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond[N_THREADS];

uint64_t detect_calls = 0;
uint64_t detect_time_ns = 0;
uint64_t recoveries = 0;

static inline uint64_t now_ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ULL + t.tv_nsec;
}

void cleanup_and_exit() {
    running = 0;
    for (int i = 0; i < N_THREADS; ++i)
        pthread_cond_broadcast(&thread_cond[i]);
}

int try_allocate(int tid, int req[M_RESOURCES]) {
    for (int r = 0; r < M_RESOURCES; ++r)
        if (req[r] > available[r]) return 0;
    for (int r = 0; r < M_RESOURCES; ++r) {
        available[r] -= req[r];
        alloc[tid][r] += req[r];
    }
    return 1;
}

void release_allocation(int tid) {
    for (int r = 0; r < M_RESOURCES; ++r) {
        if (alloc[tid][r] > 0) {
            available[r] += alloc[tid][r];
            alloc[tid][r] = 0;
        }
    }
}

int detect_cycle_and_collect(int victims[], int *victim_count) {
    int wait_for[N_THREADS][N_THREADS];
    for (int i = 0; i < N_THREADS; ++i)
        for (int j = 0; j < N_THREADS; ++j)
            wait_for[i][j] = 0;

    for (int t = 0; t < N_THREADS; ++t) {
        if (!waiting[t]) continue;
        for (int r = 0; r < M_RESOURCES; ++r) {
            if (request_vec[t][r] == 0) continue;
            for (int h = 0; h < N_THREADS; ++h) {
                if (alloc[h][r] > 0) wait_for[t][h] = 1;
            }
        }
    }

    int visited[N_THREADS], onstack[N_THREADS], stack[N_THREADS];
    for (int i = 0; i < N_THREADS; ++i) { visited[i]=0; onstack[i]=0; }
    int found = 0;
    int sp = 0;

    void dfs(int u) {
        if (found) return;
        visited[u] = 1;
        onstack[u] = 1;
        stack[sp++] = u;
        for (int v = 0; v < N_THREADS; ++v) if (wait_for[u][v]) {
            if (!visited[v]) dfs(v);
            else if (onstack[v]) {
                found = 1;
                int start = sp-1;
                while (start >= 0 && stack[start] != v) start--;
                if (start < 0) start = 0;
                *victim_count = 0;
                for (int k = start; k < sp; ++k) victims[(*victim_count)++] = stack[k];
                return;
            }
            if (found) return;
        }
        onstack[u] = 0;
        sp--;
    }

    for (int i = 0; i < N_THREADS; ++i) if (waiting[i] && !visited[i]) dfs(i);
    return found;
}

int select_victim(int victims[], int victim_count) {
    if (victim_count == 0) return -1;
    if (strategy == STRAT_RANDOM) {
        return victims[rand() % victim_count];
    } else {
        int best = victims[0];
        int min_alloc = 0;
        for (int r = 0; r < M_RESOURCES; ++r) min_alloc += alloc[best][r];
        for (int i = 1; i < victim_count; ++i) {
            int t = victims[i];
            int sum = 0;
            for (int r = 0; r < M_RESOURCES; ++r) sum += alloc[t][r];
            if (sum < min_alloc) { min_alloc = sum; best = t; }
        }
        return best;
    }
}

void detector_thread_body() {
    while (running) {
        usleep(DETECT_INTERVAL_MS * 1000);
        uint64_t start = now_ns();
        int victims[N_THREADS];
        int victim_count = 0;
        pthread_mutex_lock(&sys_mutex);
        detect_calls++;
        int found = detect_cycle_and_collect(victims, &victim_count);
        if (found && victim_count > 0) {
            int victim = select_victim(victims, victim_count);
            if (victim >= 0) {
                recoveries++;
                release_allocation(victim);
                waiting[victim] = 0;
                for (int r = 0; r < M_RESOURCES; ++r) request_vec[victim][r] = 0;
                pthread_cond_signal(&thread_cond[victim]);
            }
        }
        pthread_mutex_unlock(&sys_mutex);
        uint64_t end = now_ns();
        detect_time_ns += (end - start);
    }
}

void *detector_main(void *arg) {
    detector_thread_body();
    return NULL;
}

void simulate_work(int tid) {
    usleep((rand() % 200 + 50) * 1000);
}

void *worker(void *arg) {
    int tid = (int)(intptr_t)arg;
    while (running) {
        int req[M_RESOURCES] = {0};
        int k = (rand() % M_RESOURCES) + 1;
        for (int i = 0; i < k; ++i) {
            int r = rand() % M_RESOURCES;
            req[r] = (req[r] + 1);
            if (req[r] > MAX_INSTANCES) req[r] = MAX_INSTANCES;
        }
        pthread_mutex_lock(&sys_mutex);
        int allocated = try_allocate(tid, req);
        if (allocated) {
            pthread_mutex_unlock(&sys_mutex);
            simulate_work(tid);
            pthread_mutex_lock(&sys_mutex);
            release_allocation(tid);
            pthread_cond_broadcast(NULL);
            pthread_mutex_unlock(&sys_mutex);
            usleep((rand() % 200 + 50) * 1000);
            continue;
        } else {
            for (int r = 0; r < M_RESOURCES; ++r) request_vec[tid][r] = req[r];
            waiting[tid] = 1;
            while (waiting[tid] && running) {
                pthread_cond_wait(&thread_cond[tid], &sys_mutex);
                if (!running) break;
                if (!waiting[tid]) break;
                int ok = try_allocate(tid, request_vec[tid]);
                if (ok) { waiting[tid] = 0; for (int r=0;r<M_RESOURCES;++r) request_vec[tid][r]=0; break; }
            }
            pthread_mutex_unlock(&sys_mutex);
            if (!running) break;
            if (!waiting[tid]) {
                simulate_work(tid);
                pthread_mutex_lock(&sys_mutex);
                release_allocation(tid);
                pthread_mutex_unlock(&sys_mutex);
                usleep((rand() % 200 + 50) * 1000);
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    srand(time(NULL) ^ getpid());
    if (argc > 1) {
        if (strcmp(argv[1], "random") == 0) strategy = STRAT_RANDOM;
        else strategy = STRAT_LEAST_ALLOC;
    }
    for (int r = 0; r < M_RESOURCES; ++r) available[r] = (rand() % MAX_INSTANCES) + 1;
    for (int i = 0; i < N_THREADS; ++i) {
        for (int r = 0; r < M_RESOURCES; ++r) alloc[i][r] = request_vec[i][r] = 0;
        waiting[i] = 0;
        pthread_cond_init(&thread_cond[i], NULL);
    }
    pthread_t wthreads[N_THREADS];
    pthread_t detector;
    for (int i = 0; i < N_THREADS; ++i) pthread_create(&wthreads[i], NULL, worker, (void*)(intptr_t)i);
    pthread_create(&detector, NULL, detector_main, NULL);
    sleep(RUN_SECONDS);
    pthread_mutex_lock(&sys_mutex);
    running = 0;
    for (int i = 0; i < N_THREADS; ++i) pthread_cond_broadcast(&thread_cond[i]);
    pthread_mutex_unlock(&sys_mutex);
    for (int i = 0; i < N_THREADS; ++i) pthread_join(wthreads[i], NULL);
    pthread_join(detector, NULL);
    printf("Detection calls: %" PRIu64 "\n", detect_calls);
    printf("Total detection time: %" PRIu64 " ms\n", detect_time_ns / 1000000ULL);
    if (detect_calls) printf("Avg detection time: %.3f ms\n", (double)detect_time_ns / (double)detect_calls / 1e6);
    printf("Recoveries performed: %" PRIu64 "\n", recoveries);
    printf("Final available vector:");
    for (int r = 0; r < M_RESOURCES; ++r) printf(" %d", available[r]);
    printf("\n");
    return 0;
}
