#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
 
#define POOL_SIZE (10 * 1024 * 1024) 
#define CHUNK_SIZE 64 
#define NUM_CHUNKS (POOL_SIZE / CHUNK_SIZE) 
 
typedef struct FreeChunk { 
    struct FreeChunk* next; 
} FreeChunk; 
 
static char* pool_start = NULL; 
static FreeChunk* free_list = NULL; 
static size_t allocations_count = 0; 
static size_t deallocations_count = 0; 
static size_t internal_fragmentation = 0; 
 
void pool_init() { 
    pool_start = (char*)malloc(POOL_SIZE); 
    if (!pool_start) { 
        perror("malloc failed"); 
        exit(EXIT_FAILURE); 
    } 
 
    for (size_t i = 0; i < NUM_CHUNKS; ++i) { 
        FreeChunk* chunk = (FreeChunk*)(pool_start + i * CHUNK_SIZE); 
        chunk->next = free_list; 
        free_list = chunk; 
    } 
} 
 
void* pool_alloc(size_t size) { 
    if (size > CHUNK_SIZE) { 
        return NULL; 
    } 
 
    if (free_list == NULL) { 
        return NULL; 
    } 
 
    FreeChunk* allocated_chunk = free_list; 
    free_list = free_list->next; 
 
    allocations_count++; 
    internal_fragmentation += (CHUNK_SIZE - size); 
     
    return (void*)allocated_chunk; 
} 
 
void pool_free(void* ptr) { 
    if (ptr == NULL) { 
        return; 
    } 
 
    FreeChunk* chunk = (FreeChunk*)ptr; 
    chunk->next = free_list; 
 
    free_list = chunk; 
 
    deallocations_count++; 
} 
 
void pool_stats() { 
    size_t available_chunks = 0; 
    FreeChunk* current = free_list; 
    while (current != NULL) { 
        available_chunks++; 
        current = current->next; 
    } 
     
    printf("--- Custom Allocator Stats ---\n"); 
    printf("Total Pool Size: %lu bytes\n", (long unsigned int)POOL_SIZE); 
    printf("Chunk Size: %lu bytes\n", (long unsigned int)CHUNK_SIZE); 
    printf("Total Chunks: %lu\n", (long unsigned int)NUM_CHUNKS); 
    printf("Allocations: %lu\n", (long unsigned int)allocations_count); 
    printf("Deallocations: %lu\n", (long unsigned int)deallocations_count); 
    printf("Available Chunks: %lu\n", (long unsigned int)available_chunks); 
    printf("Wasted Space (Internal Fragmentation): %lu bytes\n", (long unsigned int)internal_fragmentation); 
    printf("--- End Stats ---\n"); 
} 
 
void pool_destroy() { 
    if (pool_start != NULL) { 
        free(pool_start); 
        pool_start = NULL; 
        free_list = NULL; 
    } 
} 
 
int main() { 
    pool_init(); 
     
    void* p1 = pool_alloc(10); 
    void* p2 = pool_alloc(60); 
    void* p3 = pool_alloc(32); 
     
    pool_stats(); 
     
    pool_free(p2); 
    pool_free(p1); 
     
    void* p4 = pool_alloc(1); 
     
    pool_stats(); 
 
    pool_destroy(); 
     
    return 0; 
}
