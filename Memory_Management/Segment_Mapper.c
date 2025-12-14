                 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
 
int global_initialized = 42; 
int global_uninitialized; 
 
void my_function() {} 
 
int main() { 
    int local_var = 10; 
    int *heap_var = (int *)malloc(sizeof(int)); 
    *heap_var = 99; 
 
    printf("Variable\tAddress\t\tPredicted\tActual\tPermissions\tSize(KB)\n"); 
    printf("-------------------------------------------------------------------------------\n"); 
 
    FILE *fp = fopen("/proc/self/maps", "r"); 
    char line[256]; 
 
    while (fgets(line, sizeof(line), fp)) { 
        unsigned long start, end; 
        char perms[5], name[100] = ""; 
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %s", &start, &end, perms, name); 
 
        if (strstr(name, ".text") || strstr(line, "r-xp")) { 
            if ((unsigned long)my_function >= start && (unsigned long)my_function <= 
end) 
                printf("my_function\t%p\tText\tText\t%s\t%lu\n", (void *)my_function, 
perms, (end - start)/1024); 
        } 
 
        if (strstr(name, ".data") || strstr(line, "rw-p")) { 
            if ((unsigned long)&global_initialized >= start && (unsigned 
long)&global_initialized <= end) 
 
 
                printf("global_initialized\t%p\tData\tData\t%s\t%lu\n", (void 
*)&global_initialized, perms, (end - start)/1024); 
        } 
 
        if (strstr(name, ".bss") || strstr(line, "rw-p")) { 
            if ((unsigned long)&global_uninitialized >= start && (unsigned 
long)&global_uninitialized <= end) 
                printf("global_uninitialized\t%p\tBSS\tBSS\t%s\t%lu\n", (void 
*)&global_uninitialized, perms, (end - start)/1024); 
        } 
 
        if (strstr(name, "[heap]")) { 
            if ((unsigned long)heap_var >= start && (unsigned long)heap_var <= end) 
                printf("heap_var\t%p\tHeap\tHeap\t%s\t%lu\n", (void *)heap_var, perms, 
(end - start)/1024); 
        } 
        if (strstr(name, "[stack]")) { 
            if ((unsigned long)&local_var >= start && (unsigned  
 
 
long)&local_var <= end) 
                printf("local_var\t%p\tStack\tStack\t%s\t%lu\n", (void *)&local_var, perms, 
(end - start)/1024); 
        } 
    } 
 
    fclose(fp); 
    free(heap_var); 
    return 0; 
} 
