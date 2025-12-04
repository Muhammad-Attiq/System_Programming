#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define SIZE 1000
#define CHILDREN 4

// Simple comparison function for qsort
int cmp(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Merge two sorted arrays
void merge(int *arr1, int n1, int *arr2, int n2, int *result) {
    int i=0, j=0, k=0;
    while (i<n1 && j<n2) {
        if (arr1[i]<=arr2[j]) result[k++] = arr1[i++];
        else result[k++] = arr2[j++];
    }
    while(i<n1) result[k++] = arr1[i++];
    while(j<n2) result[k++] = arr2[j++];
}

// Merge 4 sorted arrays
void merge4(int *seg1, int *seg2, int *seg3, int *seg4, int size, int *result) {
    int temp1[SIZE/2], temp2[SIZE/2];
    merge(seg1, size, seg2, size, temp1);
    merge(seg3, size, seg4, size, temp2);
    merge(temp1, 2*size, temp2, 2*size, result);
}

int main() {
    int arr[SIZE];
    int segment_size = SIZE / CHILDREN;
    int pipes[CHILDREN][2];

    // Seed random
    srand(time(NULL));

    // Generate random array
    for(int i=0;i<SIZE;i++) arr[i] = rand()%10000;

    // ----------------------------
    // Single-process sort timing
    // ----------------------------
    int single_arr[SIZE];
    for(int i=0;i<SIZE;i++) single_arr[i] = arr[i];

    clock_t start_single = clock();
    qsort(single_arr, SIZE, sizeof(int), cmp);
    clock_t end_single = clock();
    double time_single = (double)(end_single - start_single)/CLOCKS_PER_SEC;
    printf("Single-process sort time: %.6f seconds\n", time_single);

    // ----------------------------
    // Parallel multi-process sort
    // ----------------------------

    // Create pipes
    for(int i=0;i<CHILDREN;i++) {
        if(pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(1);
        }
    }

    for(int i=0;i<CHILDREN;i++) {
        pid_t pid = fork();
        if(pid < 0) {
            perror("fork");
            exit(1);
        }

        if(pid == 0) { // Child
            close(pipes[i][0]); // close read end
            int start = i*segment_size;
            int temp[segment_size];
            for(int j=0;j<segment_size;j++)
                temp[j] = arr[start+j];

            qsort(temp, segment_size, sizeof(int), cmp);

            write(pipes[i][1], temp, sizeof(int)*segment_size);
            close(pipes[i][1]);
            exit(0);
        } else { // Parent
            close(pipes[i][1]); // close write end
        }
    }

    // Parent reads sorted segments
    int seg[CHILDREN][segment_size];
    for(int i=0;i<CHILDREN;i++) {
        read(pipes[i][0], seg[i], sizeof(int)*segment_size);
        close(pipes[i][0]);
    }

    // Wait for children
    for(int i=0;i<CHILDREN;i++) wait(NULL);

    // Merge 4 segments
    int sorted[SIZE];
    clock_t start_parallel = clock();
    merge4(seg[0], seg[1], seg[2], seg[3], segment_size, sorted);
    clock_t end_parallel = clock();
    double time_parallel = (double)(end_parallel - start_parallel)/CLOCKS_PER_SEC;

    printf("Parallel merge time: %.6f seconds\n", time_parallel);
    printf("Total parallel sort time (including merge): %.6f seconds\n", time_parallel); // merge is main parallel portion

    // Speedup
    double speedup = time_single / time_parallel;
    printf("Speedup achieved: %.2f\n", speedup);

    // Optional: print first 20 sorted numbers
    printf("First 20 numbers in sorted array:\n");
    for(int i=0;i<20;i++) printf("%d ", sorted[i]);
    printf("\n");

    return 0;
}
