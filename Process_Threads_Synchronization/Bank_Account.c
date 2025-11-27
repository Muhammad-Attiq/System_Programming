#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10
#define NUM_OPERATIONS 100000

long balance = 0;            // shared bank balance
pthread_mutex_t lock;        // mutex lock

void* perform_transactions(void* arg) {
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        int amount = (rand() % 100) + 1;   // random amount (1–100)
        int operation = rand() % 2;        // 0 = deposit, 1 = withdraw

        pthread_mutex_lock(&lock);

        if (operation == 0) {
            // Deposit
            balance += amount;
        } else {
            // Withdraw (only if balance is enough)
            if (balance >= amount) {
                balance -= amount;
            }
        }

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, perform_transactions, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    printf("Final synchronized balance: %ld\n", balance);
    return 0;
}
