#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/resource.h> 
#include <signal.h> 
#include <unistd.h> 
 
#define STACK_ALLOC_SIZE 10 * 1024 
 
long long current_depth = 0; 
 
void stack_overflow_handler(int sig) { 
    printf("\n\n--- Stack Overflow Detected (Signal %d: SIGSEGV) ---\n", sig); 
    printf("CRASH REPORT: Recursion depth before crash: %lld\n", current_depth); 
    exit(EXIT_FAILURE); 
} 
 
void recursive_allocator() { 
    char local_array[STACK_ALLOC_SIZE]; 
    memset(local_array, 0, STACK_ALLOC_SIZE);  
    current_depth++; 
    recursive_allocator(); 
} 
 
int main() { 
    struct rlimit rl; 
    int result; 
    printf("--- Task 7: Stack Limits and Recursion ---\n"); 
    printf("Process PID: %d\n\n", getpid()); 
    
    signal(SIGSEGV, stack_overflow_handler); 
 
    result = getrlimit(RLIMIT_STACK, &rl); 
    if (result == 0) { 
        printf(" Default Stack Limit (RLIMIT_STACK):\n"); 
        printf("   - Soft Limit (rlim_cur): %ld bytes\n", rl.rlim_cur); 
        printf("   - Hard Limit (rlim_max): %ld bytes\n", rl.rlim_max); 
        printf("\n"); 
    } 
    else { 
        perror("getrlimit failed"); 
        return EXIT_FAILURE; 
    } 
 
    const rlim_t new_soft_limit = 64 * 1024 * 1024; 
     
    rl.rlim_cur = new_soft_limit; 
     
    if (rl.rlim_cur > rl.rlim_max) { 
        rl.rlim_cur = rl.rlim_max; 
    } 
     
    result = setrlimit(RLIMIT_STACK, &rl); 
     
    if (result == 0) { 
        printf("\n--- Stack Limit Modification ---\n"); 
        printf("Stack soft limit successfully set to: %ld bytes\n", rl.rlim_cur); 
    } 
    else { 
        perror("setrlimit failed. Check if hard limit allows the change."); 
    } 
 
    printf("\n2. Starting recursion with **MODIFIED** stack limit...\n"); 
    current_depth = 0; 
 
    recursive_allocator();  
 
    printf("Error: Recursion did not cause a stack overflow.\n"); 
    return EXIT_SUCCESS; 
} 
