#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/mman.h> 
#include <sys/time.h> 
#include <sys/resource.h> 
#include <time.h> 
 
#define FILENAME "test_100MB_file.dat" 
#define FILE_SIZE (100 * 1024 * 1024) 
#define READ_ATTEMPTS 100000 
#define PAGE_SIZE 4096 
 
double get_time_diff(struct timeval *start, struct timeval *end) { 
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_usec - start>tv_usec) / 1000000.0; 
} 
 
 
void get_resource_usage(struct rusage *usage) { 
    if (getrusage(RUSAGE_SELF, usage) == -1) { 
        perror("getrusage"); 
    } 
} 
 
void print_resource_diff(const char *label, struct rusage *start, struct rusage *end, 
double time_sec) { 
    printf("  Time (%s): %.4f seconds\n", label, time_sec); 
    printf("  Page Faults (Minor): %ld\n", end->ru_minflt - start->ru_minflt); 
    printf("  Page Faults (Major): %ld\n", end->ru_majflt - start->ru_majflt); 
} 
 
void create_test_file() { 
    printf("--- 1. Creating %dMB test file: %s ---\n", FILE_SIZE / (1024 * 1024), FILENAME); 
    int fd = open(FILENAME, O_CREAT | O_WRONLY | O_TRUNC, 0644); 
    if (fd == -1) { 
        perror("open failed to create file"); 
        exit(EXIT_FAILURE); 
    } 
 
    char *buffer = (char *)malloc(PAGE_SIZE); 
    if (buffer == NULL) { 
        perror("malloc failed"); 
        close(fd); 
        exit(EXIT_FAILURE); 
    } 
    for (int i = 0; i < PAGE_SIZE; i++) { 
        buffer[i] = 'A' + (i % 26); 
    } 
 
    for (off_t i = 0; i < FILE_SIZE; i += PAGE_SIZE) { 
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) { 
            perror("write failed"); 
            free(buffer); 
            close(fd); 
            exit(EXIT_FAILURE); 
        } 
    } 
 
    free(buffer); 
    close(fd); 
    printf("File creation complete.\n\n"); 
} 
 
void test_read_method() { 
    printf("--- 2. Method 1: Traditional I/O (read()) ---\n"); 
    int fd; 
    char *data_buffer; 
    struct timeval start_time, end_time; 
    struct rusage start_rusage, end_rusage; 
 
    get_resource_usage(&start_rusage); 
    gettimeofday(&start_time, NULL); 
    fd = open(FILENAME, O_RDONLY); 
 
 
    if (fd == -1) { 
        perror("open for read failed"); 
        return; 
    } 
 
    data_buffer = (char *)malloc(FILE_SIZE); 
    if (data_buffer == NULL) { 
        perror("malloc failed for read buffer"); 
        close(fd); 
        return; 
    } 
     
    ssize_t bytes_read = 0; 
    while (bytes_read < FILE_SIZE) { 
        ssize_t result = read(fd, data_buffer + bytes_read, FILE_SIZE - bytes_read); 
        if (result == -1) { 
            perror("read error"); 
            free(data_buffer); 
            close(fd); 
            return; 
        } 
        if (result == 0) break; 
        bytes_read += result; 
    } 
     
    gettimeofday(&end_time, NULL); 
    get_resource_usage(&end_rusage); 
    close(fd); 
 
    printf("Load Phase (read()):\n"); 
    print_resource_diff("Load", &start_rusage, &end_rusage, get_time_diff(&start_time, &end_time)); 
 
    get_resource_usage(&start_rusage); 
    gettimeofday(&start_time, NULL); 
 
    char temp_char = 0; 
    for (int i = 0; i < READ_ATTEMPTS; i++) { 
        off_t offset = rand() % (FILE_SIZE / PAGE_SIZE) * PAGE_SIZE; 
        temp_char += data_buffer[offset]; 
    } 
     
    gettimeofday(&end_time, NULL); 
    get_resource_usage(&end_rusage); 
 
    printf("\nAccess Phase (read() buffer):\n"); 
    print_resource_diff("Access", &start_rusage, &end_rusage, get_time_diff(&start_time, &end_time)); 
    printf("  (Dummy sum check: %d)\n\n", (int)temp_char); 
 
    free(data_buffer); 
} 
 
void test_mmap_method() { 
    printf("--- 3. Method 2: Memory-Mapped I/O (mmap()) ---\n"); 
    int fd; 
    char *map_addr; 
 
 
    struct timeval start_time, end_time; 
    struct rusage start_rusage, end_rusage; 
 
    fd = open(FILENAME, O_RDONLY); 
    if (fd == -1) { 
        perror("open for mmap failed"); 
        return; 
    } 
 
    get_resource_usage(&start_rusage); 
    gettimeofday(&start_time, NULL); 
 
    map_addr = (char *)mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, 0); 
    if (map_addr == MAP_FAILED) { 
        perror("mmap failed"); 
        close(fd); 
        return; 
    } 
     
    gettimeofday(&end_time, NULL); 
    get_resource_usage(&end_rusage); 
     
    printf("Load Phase (mmap()):\n"); 
    print_resource_diff("Map Only", &start_rusage, &end_rusage, get_time_diff(&start_time, &end_time)); 
 
    get_resource_usage(&start_rusage); 
    gettimeofday(&start_time, NULL); 
 
    char temp_char = 0; 
    for (int i = 0; i < READ_ATTEMPTS; i++) { 
        off_t offset = rand() % (FILE_SIZE / PAGE_SIZE) * PAGE_SIZE; 
        temp_char += map_addr[offset]; 
    } 
     
    gettimeofday(&end_time, NULL); 
    get_resource_usage(&end_rusage); 
 
    printf("\nAccess Phase (mmap() memory):\n"); 
    print_resource_diff("Access", &start_rusage, &end_rusage, get_time_diff(&start_time, &end_time)); 
    printf("  (Dummy sum check: %d)\n", (int)temp_char); 
 
    if (munmap(map_addr, FILE_SIZE) == -1) { 
        perror("munmap failed"); 
    } 
    close(fd); 
} 
 
void examine_proc_maps() { 
    char command[256]; 
    printf("\n--- 4. Examining /proc/[pid]/maps for mmap'd region ---\n"); 
    printf("To see the mapped region, run this command in a separate terminal while the program is paused:\n"); 
    
    printf("Current PID: %d. Press ENTER to continue...", getpid()); 
 
    fgets(command, sizeof(command), stdin); 
     
    printf("\nContinuing...\n"); 
} 
 
int main(int argc, char *argv[]) { 
    srand(time(NULL));  
 
    if (argc > 1 && strcmp(argv[1], "clean") == 0) { 
        if (remove(FILENAME) == 0) { 
            printf("Cleaned up test file: %s\n", FILENAME); 
        } else { 
            perror("Error cleaning up file"); 
        } 
        return 0; 
    } 
 
    create_test_file(); 
     
    test_read_method(); 
 
    examine_proc_maps(); 
    test_mmap_method(); 
     
    printf("\n--- Comparison Summary ---\n"); 
    printf("1. Traditional read(): Fast random access after the initial, slow, full load.\n"); 
    printf("2. mmap(): Very fast initial mapping, but major page faults occur *during* random access (lazy loading).\n"); 
     
    printf("\nTo clean up the test file, run: %s clean\n", argv[0]); 
 
    return 0; 
} 
