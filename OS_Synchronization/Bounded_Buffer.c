#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
int in = 0, out = 0;

sem_t empty;  
sem_t full;   
pthread_mutex_t mutex;

void* producer(void* arg) {
    int id = *(int*)arg;
    int item;

    while (1) {
        item = rand() % 100;

        sem_wait(&empty);           
        pthread_mutex_lock(&mutex); 

        buffer[in] = item;
        printf("Producer %d produced item %d\n", id, item);
        in = (in + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);
        sem_post(&full);

        usleep((rand() % 300) * 1000);
    }
}

void* consumer(void* arg) {
    int id = *(int*)arg;
    int item;

    while (1) {
        sem_wait(&full);            
        pthread_mutex_lock(&mutex); 

        item = buffer[out];
        printf("Consumer %d consumed item %d\n", id, item);
        out = (out + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        usleep((rand() % 300) * 1000);
    }
}

int main() {
    pthread_t prod[2], cons[2];
    int p1 = 1, p2 = 2;
    int c1 = 1, c2 = 2;

    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);

    pthread_create(&prod[0], NULL, producer, &p1);
    pthread_create(&prod[1], NULL, producer, &p2);
    pthread_create(&cons[0], NULL, consumer, &c1);
    pthread_create(&cons[1], NULL, consumer, &c2);

    pthread_join(prod[0], NULL);
    pthread_join(prod[1], NULL);
    pthread_join(cons[0], NULL);
    pthread_join(cons[1], NULL);

    return 0;
}
