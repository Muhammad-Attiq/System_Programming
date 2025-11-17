#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int shared_data = 0;
int read_count = 0;

pthread_mutex_t mutex;
pthread_mutex_t rw_mutex;

void* reader(void* arg) {
    int id = *(int*)arg;
    while(1) {
        pthread_mutex_lock(&mutex);
        read_count++;
        if(read_count == 1)
            pthread_mutex_lock(&rw_mutex);
        pthread_mutex_unlock(&mutex);

        printf("Reader %d is reading value %d\n", id, shared_data);
        usleep(rand() % 500000);

        pthread_mutex_lock(&mutex);
        read_count--;
        if(read_count == 0)
            pthread_mutex_unlock(&rw_mutex);
        pthread_mutex_unlock(&mutex);

        usleep(rand() % 500000);
    }
    return NULL;
}

void* writer(void* arg) {
    int id = *(int*)arg;
    while(1) {
        pthread_mutex_lock(&rw_mutex);

        shared_data = rand() % 100;
        printf("Writer %d is writing value %d\n", id, shared_data);
        usleep(rand() % 500000);

        pthread_mutex_unlock(&rw_mutex);

        usleep(rand() % 500000);
    }
    return NULL;
}

int main() {
    pthread_t r_threads[3], w_threads[2];
    int r_ids[3] = {1, 2, 3};
    int w_ids[2] = {1, 2};

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&rw_mutex, NULL);

    for(int i = 0; i < 3; i++)
        pthread_create(&r_threads[i], NULL, reader, &r_ids[i]);

    for(int i = 0; i < 2; i++)
        pthread_create(&w_threads[i], NULL, writer, &w_ids[i]);

    for(int i = 0; i < 3; i++)
        pthread_join(r_threads[i], NULL);
    for(int i = 0; i < 2; i++)
        pthread_join(w_threads[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&rw_mutex);

    return 0;
}
