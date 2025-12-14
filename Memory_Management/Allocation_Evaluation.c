#include <stdio.h> 
#include <stdlib.h> 
#include <sys/time.h> 
#include <sys/resource.h> 
 
#define SIZE 10000 
 
long get_page_faults() { 
    struct rusage usage; 
    getrusage(RUSAGE_SELF, &usage); 
    return usage.ru_majflt + usage.ru_minflt; 
} 
 
long time_diff(struct timeval start, struct timeval end) { 
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec); 
} 
 
int main() { 
    struct timeval start, end; 
    long pf_before, pf_after; 
 
    printf("===== Dynamic vs Static Allocation =====\n\n"); 
 
    pf_before = get_page_faults(); 
    gettimeofday(&start, NULL); 
    int stack_arr[SIZE]; 
    for (int i = 0; i < SIZE; i++) stack_arr[i] = i; 
    gettimeofday(&end, NULL); 
 
 
    pf_after = get_page_faults(); 
    printf("Stack Allocation:\n"); 
    printf("Address: %p\n", (void*)stack_arr); 
    printf("Time: %ld us\n", time_diff(start, end)); 
    printf("Page faults: %ld\n", pf_after - pf_before); 
    printf("First element: %d\n\n", stack_arr[0]); 
 
    pf_before = get_page_faults(); 
    gettimeofday(&start, NULL); 
    int *malloc_arr = (int*) malloc(SIZE * sizeof(int)); 
    for (int i = 0; i < SIZE; i++) malloc_arr[i] = i; 
    gettimeofday(&end, NULL); 
    pf_after = get_page_faults(); 
    printf("malloc() Allocation:\n"); 
    printf("Address: %p\n", (void*)malloc_arr); 
    printf("Time: %ld us\n", time_diff(start, end)); 
    printf("Page faults: %ld\n", pf_after - pf_before); 
    printf("First element: %d\n\n", malloc_arr[0]); 
 
    pf_before = get_page_faults(); 
    gettimeofday(&start, NULL); 
    int *calloc_arr = (int*) calloc(SIZE, sizeof(int)); 
    gettimeofday(&end, NULL); 
    pf_after = get_page_faults(); 
    printf("calloc() Allocation:\n"); 
    printf("Address: %p\n", (void*)calloc_arr); 
    printf("Time: %ld us\n", time_diff(start, end)); 
    printf("Page faults: %ld\n", pf_after - pf_before); 
    printf("First element: %d\n\n", calloc_arr[0]); 
 
    free(malloc_arr); 
    free(calloc_arr); 
 
    printf("===== End of Test =====\n"); 
    return 0; 
} 
