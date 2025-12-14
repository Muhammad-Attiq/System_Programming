#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/time.h> 
#include <sys/resource.h> 
#include <string.h> 
 
#define ARRAY_SIZE_MB 500 
#define ARRAY_SIZE_BYTES (ARRAY_SIZE_MB * 1024 * 1024) 
#define INT_SIZE_BYTES 4 
#define ARRAY_SIZE_INTS (ARRAY_SIZE_BYTES / INT_SIZE_BYTES) 
#define NUM_STAGES 4 
// Function to read VmSize and VmRSS from /proc/[pid]/status 
 
void read_vm_stats(long* VmSize, long* VmRSS) { 
    char path[100]; 
    sprintf(path, "/proc/%d/status", getpid()); 
    FILE* file = fopen(path, "r"); 
    if (!file) { 
        perror("Failed to open /proc/[pid]/status"); 
        *VmSize = -1; 
        *VmRSS = -1; 
        return; 
    } 
 
    char line[128]; 
    *VmSize = 0; 
    *VmRSS = 0; 
 
    while (fgets(line, sizeof(line), file)) { 
        if (strncmp(line, "VmSize:", 7) == 0) { 
            sscanf(line + 7, "%ld kB", VmSize); 
        } else if (strncmp(line, "VmRSS:", 6) == 0) { 
            sscanf(line + 6, "%ld kB", VmRSS); 
        } 
    } 
    fclose(file); 
} 
 
// Function to get page fault counts 
long get_minor_faults() { 
    struct rusage usage; 
    if (getrusage(RUSAGE_SELF, &usage) == -1) { 
        perror("getrusage failed"); 
        return -1; 
    } 
    return usage.ru_minflt; // Minor page faults (demand paging in action) 
} 
 
void print_stats(const char* stage_name, long prev_rss, long initial_faults) { 
    long current_vm_size, current_vm_rss, current_faults; 
     
    read_vm_stats(&current_vm_size, &current_vm_rss); 
    current_faults = get_minor_faults(); 
 
    printf("\n--- %s ---\n", stage_name); 
     
    // Monitors VmSize and VmRSS 
    printf("VmSize (Virtual Memory): %ld kB\n", current_vm_size); 
    printf("VmRSS (Resident Set Size): %ld kB\n", current_vm_rss); 
 
    // Verifies that VmSize > VmRSS 
    const char* comparison = (current_vm_size > current_vm_rss) ? "Verified" : "Failed"; 
    printf("VmSize > VmRSS: %s\n", comparison); 
     
    // Records page faults 
    long faults_since_start = current_faults - initial_faults; 
    printf("Page Faults (since start): %ld\n", faults_since_start); 
 
    // Calculates Resident Set Size (RSS) growth rate (since previous stage) 
 
    if (prev_rss != -1) { 
        long rss_increase = current_vm_rss - prev_rss; 
        printf("RSS Increase (from prev. stage): %ld kB\n", rss_increase); 
        if (prev_rss > 0) { 
            double growth_rate = (double)rss_increase / prev_rss * 100.0; 
            printf("RSS Growth Rate (from prev. stage): %.2f %%\n", growth_rate); 
        } 
    } 
} 
 
int main() { 
    int* array; 
    long initial_faults; 
    long prev_rss = -1; 
    long access_points[NUM_STAGES] = {0, 25, 50, 100}; 
    const char* stage_names[NUM_STAGES] = { 
        "After Allocation (Before Access)", 
        "After 25%% Access", 
        "After 50%% Access", 
        "After 100%% Access" 
    }; 
 
    // 1. Allocates a very large array (500MB) using malloc() 
    array = (int*)malloc(ARRAY_SIZE_BYTES); 
    if (array == NULL) { 
        perror("Failed to allocate memory"); 
        return 1; 
    } 
 
    // Capture initial fault count before any access 
    initial_faults = get_minor_faults(); 
 
    // 2. Does NOT access the memory initially 
    // 3. Monitors VmSize and VmRSS 
    print_stats(stage_names[0], prev_rss, initial_faults); 
    read_vm_stats(NULL, &prev_rss); // Update prev_rss for next stage 
 
    // Loop through access stages (25%, 50%, 100%) 
    for (int i = 1; i < NUM_STAGES; ++i) { 
        long percentage = access_points[i]; 
        long access_limit = (ARRAY_SIZE_INTS * percentage) / 100; 
 
        // Access the array up to the current limit to trigger demand paging 
        for (long j = 0; j < access_limit; j += (1024 * 4 / INT_SIZE_BYTES)) {  
            // Access in chunks to trigger a page fault for a new page,  
            // a step of 1024 * 4 bytes (4kB typical page size) is used here. 
            array[j] = j;  
        } 
 
        // Monitors VmSize and VmRSS / Records page faults / Calculates RSS growth 
        print_stats(stage_names[i], prev_rss, initial_faults); 
        read_vm_stats(NULL, &prev_rss); // Update prev_rss for next stage 
    } 
 
    free(array); 
    return 0; }
