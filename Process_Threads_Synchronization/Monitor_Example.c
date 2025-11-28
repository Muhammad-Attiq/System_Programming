#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS 20

typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out, count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} MonitorBuffer;

void buffer_init(MonitorBuffer* m) {
    m->in = m->out = m->count = 0;
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->not_full, NULL);
    pthread_cond_init(&m->not_empty, NULL);
}

void put(MonitorBuffer* m, int item) {
    pthread_mutex_lock(&m->mutex);
    while (m->count == BUFFER_SIZE)
        pthread_cond_wait(&m->not_full, &m->mutex);
    m->buffer[m->in] = item;
    m->in = (m->in + 1) % BUFFER_SIZE;
    m->count++;
    pthread_cond_signal(&m->not_empty);
    pthread_mutex_unlock(&m->mutex);
}

int get(MonitorBuffer* m) {
    pthread_mutex_lock(&m->mutex);
    while (m->count == 0)
        pthread_cond_wait(&m->not_empty, &m->mutex);
    int item = m->buffer[m->out];
    m->out = (m->out + 1) % BUFFER_SIZE;
    m->count--;
    pthread_cond_signal(&m->not_full);
    pthread_mutex_unlock(&m->mutex);
    return item;
}

MonitorBuffer mb;

void* producer(void* arg) {
    for (int i = 0; i < ITEMS; i++)
        put(&mb, i);
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < ITEMS; i++)
        get(&mb);
    return NULL;
}

typedef struct {
    int read_count;
    pthread_mutex_t mutex;
    pthread_cond_t ok_to_read;
    pthread_cond_t ok_to_write;
    int writing;
} MonitorRW;

void rw_init(MonitorRW* m) {
    m->read_count = 0;
    m->writing = 0;
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->ok_to_read, NULL);
    pthread_cond_init(&m->ok_to_write, NULL);
}

void start_read(MonitorRW* m) {
    pthread_mutex_lock(&m->mutex);
    while (m->writing)
        pthread_cond_wait(&m->ok_to_read, &m->mutex);
    m->read_count++;
    pthread_mutex_unlock(&m->mutex);
}

void end_read(MonitorRW* m) {
    pthread_mutex_lock(&m->mutex);
    m->read_count--;
    if (m->read_count == 0)
        pthread_cond_signal(&m->ok_to_write);
    pthread_mutex_unlock(&m->mutex);
}

void start_write(MonitorRW* m) {
    pthread_mutex_lock(&m->mutex);
    while (m->writing || m->read_count > 0)
        pthread_cond_wait(&m->ok_to_write, &m->mutex);
    m->writing = 1;
    pthread_mutex_unlock(&m->mutex);
}

void end_write(MonitorRW* m) {
    pthread_mutex_lock(&m->mutex);
    m->writing = 0;
    pthread_cond_broadcast(&m->ok_to_read);
    pthread_cond_signal(&m->ok_to_write);
    pthread_mutex_unlock(&m->mutex);
}

MonitorRW rw_monitor;
int shared_data = 0;

void* reader_thread(void* arg) {
    for (int i = 0; i < 50; i++) {
        start_read(&rw_monitor);
        int x = shared_data;
        end_read(&rw_monitor);
    }
    return NULL;
}

void* writer_thread(void* arg) {
    for (int i = 0; i < 50; i++) {
        start_write(&rw_monitor);
        shared_data++;
        end_write(&rw_monitor);
    }
    return NULL;
}

int main() {
    buffer_init(&mb);
    rw_init(&rw_monitor);

    pthread_t p, c, r1, r2, w;

    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);

    pthread_create(&r1, NULL, reader_thread, NULL);
    pthread_create(&r2, NULL, reader_thread, NULL);
    pthread_create(&w, NULL, writer_thread, NULL);

    pthread_join(r1, NULL);
    pthread_join(r2, NULL);
    pthread_join(w, NULL);

    return 0;
}
