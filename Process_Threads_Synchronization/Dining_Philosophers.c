#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PHILOSOPHERS 5
#define EAT_COUNT 50   // Number of times each philosopher eats

pthread_mutex_t forks[NUM_PHILOSOPHERS]; 
pthread_mutex_t waiter = PTHREAD_MUTEX_INITIALIZER; // used in arbitrator solution

typedef enum {
    RESOURCE_HIERARCHY,
    ARBITRATOR,
    TRY_LOCK
} approach_t;

approach_t APPROACH = RESOURCE_HIERARCHY;

// Utility: get current timestamp (microseconds)
long long timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

void think(int id) {
    usleep(1000 + rand() % 1000); // simulate thinking
}

void eat(int id) {
    usleep(1000 + rand() % 1000); // simulate eating
}

// ----------------------- APPROACH 1: RESOURCE HIERARCHY -----------------------
void pickup_forks_ordered(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    // enforce ordering
    if (left < right) {
        pthread_mutex_lock(&forks[left]);
        pthread_mutex_lock(&forks[right]);
    } else {
        pthread_mutex_lock(&forks[right]);
        pthread_mutex_lock(&forks[left]);
    }
}

void putdown_forks_ordered(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    pthread_mutex_unlock(&forks[left]);
    pthread_mutex_unlock(&forks[right]);
}

// ----------------------- APPROACH 2: ARBITRATOR (WAITER) ----------------------
void pickup_forks_waiter(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    pthread_mutex_lock(&waiter);  // only waiter controls access
    pthread_mutex_lock(&forks[left]);
    pthread_mutex_lock(&forks[right]);
    pthread_mutex_unlock(&waiter);
}

void putdown_forks_waiter(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    pthread_mutex_unlock(&forks[left]);
    pthread_mutex_unlock(&forks[right]);
}

// ----------------------- APPROACH 3: TRY-LOCK (NONBLOCKING) -------------------
void pickup_forks_trylock(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    while (1) {
        if (pthread_mutex_trylock(&forks[left]) == 0) {
            if (pthread_mutex_trylock(&forks[right]) == 0) {
                return;  // success
            } else {
                pthread_mutex_unlock(&forks[left]);
            }
        }
        // avoid busy waiting
        usleep(100);
    }
}

void putdown_forks_trylock(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    pthread_mutex_unlock(&forks[left]);
    pthread_mutex_unlock(&forks[right]);
}


// ----------------------- THREAD FUNCTION -----------------------
void* philosopher(void* arg) {
    int id = *(int*)arg;

    for (int i = 0; i < EAT_COUNT; i++) {
        think(id);

        // pick up based on chosen approach
        switch (APPROACH) {
            case RESOURCE_HIERARCHY: pickup_forks_ordered(id); break;
            case ARBITRATOR: pickup_forks_waiter(id); break;
            case TRY_LOCK: pickup_forks_trylock(id); break;
        }

        eat(id);

        // put down forks
        switch (APPROACH) {
            case RESOURCE_HIERARCHY: putdown_forks_ordered(id); break;
            case ARBITRATOR: putdown_forks_waiter(id); break;
            case TRY_LOCK: putdown_forks_trylock(id); break;
        }
    }

    return NULL;
}


// ----------------------- MAIN FUNCTION -----------------------
int main() {
    srand(time(NULL));

    pthread_t threads[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];

    printf("Dining Philosophers Simulation\n");
    printf("Approach: ");
    if (APPROACH == RESOURCE_HIERARCHY) printf("RESOURCE HIERARCHY\n");
    else if (APPROACH == ARBITRATOR) printf("ARBITRATOR (WAITER)\n");
    else printf("TRY-LOCK\n");

    // initialize forks
    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_mutex_init(&forks[i], NULL);

    long long start = timestamp();

    // create philosophers
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);
    }

    // join threads
    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_join(threads[i], NULL);

    long long end = timestamp();

    printf("Total execution time: %lld microseconds\n", end - start);

    return 0;
}
