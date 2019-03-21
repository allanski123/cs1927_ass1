#include <stdio.h> 
#include <stdlib.h> 
#include <assert.h> 
#include <stdint.h> 
#include "allocator.h" 
#include "allocator.c" 

void test_vlad_init(); 
void test_vlad_malloc(); 
void test_vlad_free(); 
void test_vlad_end(); 
void test_vlad_stats(); 

int main(int argc, char **argv) { 
printf("Testing initialization and end...\n"); 
test_vlad_init(); 
printf("Testing malloc...\n"); 
test_vlad_malloc(); 
printf("Testing free...\n"); 
test_vlad_free(); 
printf("All tests passed!\n"); 
return EXIT_SUCCESS; 
} 


void test_vlad_init() { 
vlad_init(1000); 
assert(memory != NULL); 
assert(free_list_ptr == 0); 
assert(memory_size == 1024); 
assert(strategy == BEST_FIT); 
free_header_t header = ((free_header_t *) memory)[0]; 
assert(header.magic == MAGIC_FREE); 
assert(header.size == 1024); 
assert(header.next == 0); 
assert(header.prev == 0); 
vlad_end(); 
assert(memory == NULL); 
} 

void test_vlad_malloc() { 
vlad_init(1000); 
printf("==Allocing an undersize pointer\n"); 
byte *ptr_1 = vlad_malloc(4); 
assert(ptr_1 != NULL); 
alloc_header_t alloc_1 = ((alloc_header_t *)memory)[0]; 
assert(alloc_1.magic == MAGIC_ALLOC); 
assert(alloc_1.size == 16); 
free_header_t free_1 = ((free_header_t *)&(memory[16]))[0]; 
assert(free_1.magic == MAGIC_FREE); 
assert(free_1.size == 1008); 
assert(free_1.next == 16); 
assert(free_1.prev == 16); 

printf("==Allocing a pointer which is not a multiple of 4\n"); 
byte *ptr_2 = vlad_malloc(33); 
assert(ptr_2 != NULL); 
alloc_header_t alloc_2 = ((alloc_header_t *)&(memory[16]))[0]; 
assert(alloc_2.magic == MAGIC_ALLOC); 
assert(alloc_2.size == 44); 

printf("==Attempting to alloc a pointer which is too large"); 
byte *ptr_3 = vlad_malloc(10000); 
assert(ptr_3 == NULL); 

printf("==Attempting to alloc a pointer without enough free space"); 
byte *ptr_4 = vlad_malloc(960); 
assert(ptr_4 == NULL); 

printf("==Attempting to alloc a pointer which would leave no free space"); 
byte *ptr_5 = vlad_malloc(956); 
assert(ptr_5 == NULL); 

vlad_stats(); 
vlad_end(); 
} 

void test_vlad_free() { 
vlad_init(1000); 
printf("==Allocing and freeing a single pointer\n"); 
byte *ptr_1 = vlad_malloc(1); 
alloc_header_t alloc_1 = ((alloc_header_t *)memory)[0]; 
assert(alloc_1.magic == MAGIC_ALLOC); 
assert(alloc_1.size == 16); 
vlad_free(ptr_1); 
vlad_stats(); 
free_header_t free_1 = ((free_header_t *)memory)[0]; 
assert(free_1.magic == MAGIC_FREE); 
assert(free_1.size == 1024); 
assert(free_1.next == 0); 
assert(free_1.prev == 0); 

printf("==Allocing 3 pointers to have a split\n"); 
byte *ptr_2 = vlad_malloc(40); 
byte *ptr_3 = vlad_malloc(40); 
byte *ptr_4 = vlad_malloc(40); 

printf("==Freeing first pointer\n"); 
vlad_free(ptr_2); 
vlad_stats(); 
free_header_t *free_2 = (free_header_t *)memory; 
assert(free_2->magic == MAGIC_FREE); 
assert(free_2->size == 48); 
assert(free_2->next == 144); 
assert(free_2->prev == 144); 

printf("==Freeing second pointer\n"); 
vlad_free(ptr_4); 
free_header_t *free_4 = (free_header_t *)&memory[96]; 
assert(free_4->magic == MAGIC_FREE); 
assert(free_4->size == 928); 
assert(free_2->next == 96); 
assert(free_2->prev == 96); 
assert(free_4->next == 0); 
assert(free_4->prev == 0); 
vlad_stats(); 

printf("==Freeing third pointer\n"); 
vlad_free(ptr_3); 
assert(free_2->size == 1024); 
assert(free_2->next == 0); 
assert(free_2->prev == 0); 
vlad_stats(); 
}

