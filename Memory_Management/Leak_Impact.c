#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/resource.h> 
#include <time.h> 
 
#define LEAK_SIZE_MB 10 
#define MAX_ITER 50 
 
void get_memory_info(long *vm_size, long *vm_rss, long *vm_data) { 
    FILE *f = fopen("/proc/self/status", "r"); 
    if (!f) return; 
    char line[256]; 
    while (fgets(line, sizeof(line), f)) { 
        if (strncmp(line, "VmSize:", 7) == 0) 
            sscanf(line + 7, "%ld", vm_size); 
        if (strncmp(line, "VmRSS:", 6) == 0) 
            sscanf(line + 6, "%ld", vm_rss); 
        if (strncmp(line, "VmData:", 7) == 0) 
            sscanf(line + 7, "%ld", vm_data); 
    } 
    fclose(f); 
} 
 
int main() { 
    void *leaks[MAX_ITER] = {0}; 
    long vm_size, vm_rss, vm_data; 
 
 
    struct rusage usage; 
    time_t start_time = time(NULL); 
 
    printf("Iter\tVmSize(KB)\tVmRSS(KB)\tVmData(KB)\tMajorPF\tMinorPF\n"); 
 
    for (int i = 0; i < MAX_ITER; i++) { 
        leaks[i] = malloc(LEAK_SIZE_MB * 1024 * 1024); 
        if (!leaks[i]) { 
            printf("Memory allocation failed at iteration %d\n", i); 
            break; 
        } 
 
        get_memory_info(&vm_size, &vm_rss, &vm_data); 
        getrusage(RUSAGE_SELF, &usage); 
 
        printf("%d\t%ld\t\t%ld\t\t%ld\t\t%ld\t%ld\n", 
               i+1, vm_size, vm_rss, vm_data, 
               usage.ru_majflt, usage.ru_minflt); 
 
        usleep(100000); 
    } 
    time_t end_time = time(NULL); 
 
    double leak_rate = (LEAK_SIZE_MB * MAX_ITER) / (double)(end_time - start_time); 
    printf("\nApproximate leak rate: %.2f MB/sec\n", leak_rate); 
 
    getchar(); 
    return 0; 
}
