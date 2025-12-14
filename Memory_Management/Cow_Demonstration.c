#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/wait.h> 
#include <sys/resource.h> 
#include <string.h> 
 
#define ARRAY_SIZE (50 * 1024 * 1024) 
 
void print_memory_usage(pid_t pid, const char* context) { 
    char path[128]; 
    char line[256]; 
    FILE *fp; 
    sprintf(path, "/proc/%d/status", pid); 
    fp = fopen(path, "r"); 
 
    if (fp == NULL) { 
        return; 
    } 
 
    printf("[%s] PID %d Memory Status:\n", context, pid); 
    // *** FIX: Added the file pointer 'fp' as the third argument to fgets *** 
    while (fgets(line, sizeof(line), fp) != NULL) {  
        if (strstr(line, "VmRSS:") != NULL) { 
            printf("\t%s", line); 
            break;     } 
 } 
    fclose(fp); 
} 
 
void print_page_faults(const char* context) { 
    struct rusage usage; 
    if (getrusage(RUSAGE_SELF, &usage) == 0) { 
        printf("[%s] Minor Page Faults: %ld\n", context, usage.ru_minflt); 
    } 
} 
 
int main() { 
    printf("--- COW Demonstration Started ---\n"); 
 
    printf("\n[Parent] Allocating 50 MB array...\n"); 
    int *large_array = (int*)malloc(ARRAY_SIZE); 
    if (large_array == NULL) { 
        return 1; 
    } 
 
    printf("[Parent] Initializing array...\n"); 
    for (int i = 0; i < ARRAY_SIZE / sizeof(int); ++i) { 
        large_array[i] = i; 
    } 
    printf("[Parent] Initialization complete.\n"); 
 
    print_memory_usage(getpid(), "Parent BEFORE fork"); 
    print_page_faults("Parent BEFORE fork"); 
 
    printf("\n[Parent] Forking child process...\n"); 
    pid_t pid = fork(); 
 
    if (pid < 0) { 
        free(large_array); 
        return 1; 
    }  
     
    if (pid == 0) { 
        pid_t child_pid = getpid(); 
         
        printf("\n--- Child Process (PID: %d) ---\n", child_pid); 
 
        print_memory_usage(child_pid, "Child AFTER fork, BEFORE write");  
        print_page_faults("Child AFTER fork, BEFORE write"); 
         
        printf("[Child] Reading first element: %d\n", large_array[0]); 
 
        printf("[Child] Writing to element 0 (COW expected)...\n"); 
        large_array[0] = 99999;  
         
        print_memory_usage(child_pid, "Child AFTER write"); 
        print_page_faults("Child AFTER write"); 
 
        printf("--- Child Process Exiting ---\n"); 
        free(large_array); 
        exit(0); 
    }  
     
    else { 
        waitpid(pid, NULL, 0); 
 
        printf("\n--- Parent Process (PID: %d) ---\n", getpid()); 
        printf("[Parent] Checking array value after child write: %d (Should be unchanged)\n", large_array[0]); 
        print_memory_usage(getpid(), "Parent AFTER child exit"); 
        print_page_faults("Parent AFTER child exit"); 
 
        printf("--- COW Demonstration Finished ---\n"); 
        free(large_array); 
    } 
 
    return 0; 
}  
