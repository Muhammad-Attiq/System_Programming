#include <stdio.h> 
#include <stdlib.h> 
#include <sys/resource.h> 
#include <sys/time.h> 
#include <unistd.h> 
#include <time.h> 
 
#define TOTAL_SIZE (200 * 1024 * 1024) 
#define SMALL_WS_SIZE (10 * 1024 * 1024) 
#define ITERATIONS 10000000 
 
char *memory_block = NULL; 
 
void get_faults(struct rusage *usage, long *major, long *minor) { 
    if (getrusage(RUSAGE_SELF, usage) == 0) { 
        *major = usage->ru_majflt; 
        *minor = usage->ru_minflt; 
    } else { 
        perror("getrusage failed"); 
    } 
} 
 
void run_pattern(const char *name, void (*access_func)()) { 
    struct rusage start_usage, end_usage; 
    long start_major, start_minor; 
    long end_major, end_minor; 
    struct timespec start_time, end_time; 
    double elapsed_time; 
 
    printf("\n--- Running Pattern: %s ---\n", name); 
    get_faults(&start_usage, &start_major, &start_minor); 
 
 
    clock_gettime(CLOCK_MONOTONIC, &start_time); 
 
    access_func(); 
 
    clock_gettime(CLOCK_MONOTONIC, &end_time); 
    get_faults(&end_usage, &end_major, &end_minor); 
 
    long major_faults = end_major - start_major; 
    long minor_faults = end_minor - start_minor; 
 
    elapsed_time = (end_time.tv_sec - start_time.tv_sec) +  (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0; 
 
    double fault_rate = (major_faults + minor_faults) / elapsed_time; 
 
    printf("Execution Time: %.4f seconds\n", elapsed_time); 
    printf("Major Page Faults: %ld (Disk I/O)\n", major_faults); 
    printf("Minor Page Faults: %ld (In RAM)\n", minor_faults); 
    printf("Total Page Fault Rate: %.2f faults/sec\n", fault_rate); 
} 
 
void small_working_set() { 
    for (int i = 0; i < ITERATIONS; i++) { 
        int index = i % SMALL_WS_SIZE; 
        memory_block[index]++;  
    } 
} 
 
void large_working_set() { 
    for (int i = 0; i < ITERATIONS; i++) { 
        int index = i % TOTAL_SIZE; 
        memory_block[index]++; 
    } 
} 
 
void thrashing_pattern() { 
    srand(time(NULL)); 
 
    for (int i = 0; i < ITERATIONS; i++) { 
        int index = rand() % TOTAL_SIZE; 
        memory_block[index]++; 
    } 
} 
 
int main() { 
    printf("Allocating %.2f MB of memory...\n", (double)TOTAL_SIZE / (1024 * 1024)); 
    memory_block = (char *)malloc(TOTAL_SIZE); 
     
    if (memory_block == NULL) { 
        perror("Memory allocation failed"); 
        return 1; 
    } 
    printf("Allocation successful.\n"); 
 
    for (int i = 0; i < TOTAL_SIZE; i += 4096) { 
        memory_block[i] = 0; 
    } 
    run_pattern("Small Working Set (10MB)", small_working_set); 
    run_pattern("Large Working Set (200MB)", large_working_set); 
    run_pattern("Thrashing Pattern (Random 200MB)", thrashing_pattern); 
 
    free(memory_block); 
    return 0; 
}
