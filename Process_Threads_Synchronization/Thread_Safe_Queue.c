#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TSQueue;

void queue_init(TSQueue* q) {
    q->front = q->rear = NULL;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void enqueue(TSQueue* q, int value) {
    Node* node = malloc(sizeof(Node));
    node->data = value;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->rear == NULL) q->front = q->rear = node;
    else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int dequeue(TSQueue* q) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0)
        pthread_cond_wait(&q->cond, &q->mutex);

    Node* temp = q->front;
    int value = temp->data;

    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;

    q->count--;
    pthread_mutex_unlock(&q->mutex);
    free(temp);
    return value;
}

int is_empty(TSQueue* q) {
    pthread_mutex_lock(&q->mutex);
    int empty = (q->count == 0);
    pthread_mutex_unlock(&q->mutex);
    return empty;
}

int size(TSQueue* q) {
    pthread_mutex_lock(&q->mutex);
    int n = q->count;
    pthread_mutex_unlock(&q->mutex);
    return n;
}

TSQueue queue;
int PRODUCERS = 3;
int CONSUMERS = 3;
int ITEMS_PER_PRODUCER = 10000;

void* producer(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        enqueue(&queue, i + id * 1000000);
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        dequeue(&queue);
    return NULL;
}

typedef struct {
    int* arr;
    int top;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TSStack;

void stack_init(TSStack* s, int n) {
    s->arr = malloc(sizeof(int) * n);
    s->top = -1;
    s->size = n;
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
}

void stack_push(TSStack* s, int value) {
    pthread_mutex_lock(&s->mutex);
    s->arr[++s->top] = value;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
}

int stack_pop(TSStack* s) {
    pthread_mutex_lock(&s->mutex);
    while (s->top == -1)
        pthread_cond_wait(&s->cond, &s->mutex);

    int value = s->arr[s->top--];
    pthread_mutex_unlock(&s->mutex);
    return value;
}

TSStack stack;

void* stack_producer(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        stack_push(&stack, i + id * 1000000);
    return NULL;
}

void* stack_consumer(void* arg) {
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        stack_pop(&stack);
    return NULL;
}

double now() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

int main() {
    queue_init(&queue);
    stack_init(&stack, PRODUCERS * ITEMS_PER_PRODUCER + 10);

    pthread_t p[10], c[10];
    int ids[10];

    double t1 = now();
    for (int i = 0; i < PRODUCERS; i++) {
        ids[i] = i;
        pthread_create(&p[i], NULL, producer, &ids[i]);
    }
    for (int i = 0; i < CONSUMERS; i++)
        pthread_create(&c[i], NULL, consumer, NULL);

    for (int i = 0; i < PRODUCERS; i++) pthread_join(p[i], NULL);
    for (int i = 0; i < CONSUMERS; i++) pthread_join(c[i], NULL);
    double queue_time = now() - t1;

    double t2 = now();
    for (int i = 0; i < PRODUCERS; i++)
        pthread_create(&p[i], NULL, stack_producer, &ids[i]);

    for (int i = 0; i < CONSUMERS; i++)
        pthread_create(&c[i], NULL, stack_consumer, NULL);

    for (int i = 0; i < PRODUCERS; i++) pthread_join(p[i], NULL);
    for (int i = 0; i < CONSUMERS; i++) pthread_join(c[i], NULL);
    double stack_time = now() - t2;

    printf("Queue time: %f sec\n", queue_time);
    printf("Stack time: %f sec\n", stack_time);

    return 0;
}
