#include <stdlib.h> 
#include <sys/resource.h> 
#include <time.h> 
 
#define SIZE (100 * 1024 * 1024) 
#define STRIDE 1000 
 
long get_faults() { 
    struct rusage usage; 
    getrusage(RUSAGE_SELF, &usage); 
    return usage.ru_minflt + usage.ru_majflt; 
} 
 
double get_time() { 
    struct timespec ts; 
    clock_gettime(CLOCK_MONOTONIC, &ts); 
    return ts.tv_sec + ts.tv_nsec * 1e-9; 
} 
 
int main() { 
    char *arr_seq = malloc(SIZE); 
    char *arr_rand = malloc(SIZE); 
    char *arr_stride = malloc(SIZE); 
 
 
    for (size_t i = 0; i < SIZE; i += 4096) { 
        arr_seq[i] = 0; 
        arr_rand[i] = 0; 
        arr_stride[i] = 0; 
    } 
 
    long pf_before; 
    long pf_after; 
    double t_before; 
    double t_after; 
 
    pf_before = get_faults(); 
    t_before = get_time(); 
    for (size_t i = 0; i < SIZE; i++) 
        arr_seq[i]++; 
    t_after = get_time(); 
    pf_after = get_faults(); 
    printf("Sequential Access:\nTime: %.6f sec\nPage Faults: %ld\n\n", t_after - 
t_before, pf_after - pf_before); 
 
    pf_before = get_faults(); 
    t_before = get_time(); 
    for (size_t i = 0; i < SIZE; i++) { 
        size_t index = rand() % SIZE; 
        arr_rand[index]++; 
    } 
    t_after = get_time(); 
    pf_after = get_faults(); 
    printf("Random Access:\nTime: %.6f sec\nPage Faults: %ld\n\n", t_after - 
t_before, pf_after - pf_before); 
 
    pf_before = get_faults(); 
    t_before = get_time(); 
    for (size_t i = 0; i < SIZE; i += STRIDE) 
        arr_stride[i]++; 
    t_after = get_time(); 
    pf_after = get_faults(); 
    printf("Strided Access:\nTime: %.6f sec\nPage Faults: %ld\n\n", t_after - 
t_before, pf_after - pf_before); 
 
    free(arr_seq); 
    free(arr_rand); 
    free(arr_stride); 
    return 0; 
}
