#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READERS 5
#define NUM_WRITERS 3
#define READ_TIMES 5
#define WRITE_TIMES 5

int shared_data = 0;

// Track counts
int active_readers = 0;
int waiting_writers = 0;
int active_writers = 0;

// Synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_read  = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_write = PTHREAD_COND_INITIALIZER;

// -------------------- Reader Function --------------------
void* reader(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < READ_TIMES; i++) {

        pthread_mutex_lock(&mutex);

        // Writers get priority → block readers if a writer is active OR waiting
        while (active_writers > 0 || waiting_writers > 0) {
            pthread_cond_wait(&can_read, &mutex);
        }

        active_readers++;
        printf("Reader %d START reading | Active Readers = %d | Active Writers = %d\n",
               id, active_readers, active_writers);

        pthread_mutex_unlock(&mutex);

        // Simulate reading
        usleep(100000);  
        printf("Reader %d reads value = %d\n", id, shared_data);

        pthread_mutex_lock(&mutex);

        active_readers--;
        printf("Reader %d END reading | Active Readers = %d\n", id, active_readers);

        // If no reader is left → wake a writer
        if (active_readers == 0)
            pthread_cond_signal(&can_write);

        pthread_mutex_unlock(&mutex);

        usleep(100000);
    }
    return NULL;
}

// -------------------- Writer Function --------------------
void* writer(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < WRITE_TIMES; i++) {

        pthread_mutex_lock(&mutex);

        waiting_writers++;

        // Writer waits if anyone is reading or writing
        while (active_readers > 0 || active_writers > 0) {
            pthread_cond_wait(&can_write, &mutex);
        }

        waiting_writers--;
        active_writers = 1;

        printf("WRITER %d START writing | Active Writers = %d\n",
               id, active_writers);

        pthread_mutex_unlock(&mutex);

        // Simulate writing
        usleep(150000);
        shared_data++;
        printf("WRITER %d wrote value = %d\n", id, shared_data);

        pthread_mutex_lock(&mutex);

        active_writers = 0;
        printf("WRITER %d END writing\n", id);

        // Priority to writers → if writers waiting, signal writer first
        if (waiting_writers > 0)
            pthread_cond_signal(&can_write);
        else
            pthread_cond_broadcast(&can_read);

        pthread_mutex_unlock(&mutex);

        usleep(150000);
    }
    return NULL;
}

// -------------------- main() --------------------
int main() {
    pthread_t rthreads[NUM_READERS];
    pthread_t wthreads[NUM_WRITERS];
    int r_id[NUM_READERS], w_id[NUM_WRITERS];

    // Create reader threads
    for (int i = 0; i < NUM_READERS; i++) {
        r_id[i] = i + 1;
        pthread_create(&rthreads[i], NULL, reader, &r_id[i]);
    }

    // Create writer threads
    for (int i = 0; i < NUM_WRITERS; i++) {
        w_id[i] = i + 1;
        pthread_create(&wthreads[i], NULL, writer, &w_id[i]);
    }

    // Join readers
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(rthreads[i], NULL);
    }

    // Join writers
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(wthreads[i], NULL);
    }

    printf("\nFINAL shared_data value = %d\n", shared_data);
    return 0;
}
