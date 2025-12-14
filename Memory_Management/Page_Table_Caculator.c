#include <stdio.h> 
#include <stdlib.h> 
#include <math.h> 
 
void calculate_and_print(long long vas_size, int page_size_kb, int entry_size) { 
    long long page_size = (long long)page_size_kb * 1024; 
    long long num_pages = vas_size / page_size; 
     
    long long single_level_size = num_pages * entry_size; 
     
    int entries_per_page = page_size / entry_size; 
    long long num_second_level_tables = num_pages / entries_per_page; 
     
    long long first_level_size = num_second_level_tables * entry_size; 
    long long two_level_size = first_level_size + (num_second_level_tables * page_size); 
     
    double single_level_overhead_percent = ((double)single_level_size / vas_size) * 100.0; 
    double two_level_overhead_percent = ((double)two_level_size / vas_size) * 100.0; 
 
    printf("--- Page Size: %d KB ---\n", page_size_kb); 
    printf("1. Number of pages in address space: %lld\n", num_pages); 
    printf("2. Single-level page table size: %lld bytes (%.2f MB)\n", single_level_size, (double)single_level_size / (1024 * 1024)); 
    printf("3. Two-level page table size (assuming %d entries/table): %lld bytes (%.2f MB)\n", entries_per_page, two_level_size, (double)two_level_size / (1024 * 1024)); 
    printf("4. Single-level overhead percentage: %.6f %%\n", single_level_overhead_percent); 
    printf("5. Two-level overhead percentage: %.6f %%\n", two_level_overhead_percent); 
} 
 
int main() { 
    int page_sizes[] = {1, 4, 16, 64}; 
    int num_page_sizes = sizeof(page_sizes) / sizeof(page_sizes[0]); 
     
    long long vas_size = 4LL * 1024 * 1024 * 1024;  
    int entry_size = 4;  
 
    printf("### Page Table Size Calculation ###\n"); 
    printf("Virtual Address Space Size (V): 4 GB\n"); 
    printf("Page Table Entry Size (E): %d bytes\n\n", entry_size); 
     
    for (int i = 0; i < num_page_sizes; i++) { 
        calculate_and_print(vas_size, page_sizes[i], entry_size); 
    } 
     
    printf("\n--- Analysis Required ---\n"); 
    printf("Comparison (1KB vs 64KB Page Size):\n"); 
    printf("* 1KB pages require 4 times as many entries as 4KB pages, leading to a much larger page table overhead (%.2f MB).\n", (double)(vas_size / (1LL * 1024) * entry_size) / (1024 * 1024)); 
    printf("* 64KB pages significantly reduce the page table size, but increase internal fragmentation.\n"); 
    printf("\nMulti-level Page Tables:\n"); 
 
    printf("* Modern systems use multi-level page tables (like two-level) because they allow the page table structure to be stored non-contiguously, page by page.\n"); 
    printf("* This is crucial for 64-bit systems where a single-level page table would be enormous (e.g., 256 TB V.A.S. / 4KB page * 8 bytes entry = 512 GB overhead) and cannot be allocated entirely in physical memory.\n"); 
    printf("* Multi-level tables only allocate the parts of the page table corresponding to the active memory regions, dramatically reducing memory overhead for sparse address spaces.\n"); 
 
    return 0; 
}
