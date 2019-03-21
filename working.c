//
//  COMP1927 Assignment 1 - Vlad: the memory allocator
//  allocator.c ... implementation
//
//  Created by Liam O'Connor on 18/07/12.
//  Modified by John Shepherd in August 2014, August 2015
//  Copyright (c) 2012-2015 UNSW. All rights reserved.
//

/*

Milestones & Goals + progress:

** To do ** 
1) Implement vlad_init() & vlad_end() - Will need highest power calculator function ** Mostly Complete !!
2) Implement vlad_malloc() function - Will need to learn pointer arithmetic
3) Implement vlad_free() function
4) Implement vlad_merge to merge adjacent regions together - essentially take two smaller free bits of memory and recombine into a large chunk

*/

#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define FREE_HEADER_SIZE  sizeof(struct free_list_header)  
#define ALLOC_HEADER_SIZE sizeof(struct alloc_block_header)  
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

// my defines
#define MIN_MEMORY 8
#define TRUE 1
#define FALSE 0

#define BEST_FIT       1
#define WORST_FIT      2
#define RANDOM_FIT     3

typedef unsigned char byte;
typedef u_int32_t vsize_t;
typedef u_int32_t vlink_t;
typedef u_int32_t vaddr_t;

typedef struct free_list_header {
    u_int32_t magic;  // ought to contain MAGIC_FREE
    vsize_t size;     // # bytes in this block (including header)
    vlink_t next;     // memory[] index of next free block
    vlink_t prev;     // memory[] index of previous free block
} free_header_t;

typedef struct alloc_block_header {
    u_int32_t magic;  // ought to contain MAGIC_ALLOC
    vsize_t size;     // # bytes in this block (including header)
} alloc_header_t;

// Global data

static byte *memory = NULL;   // pointer to start of allocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]
static u_int32_t strategy;    // allocation strategy (by default BEST_FIT)

// Private functions

static void vlad_merge();
static vsize_t powerOfTwo(vsize_t);
//static vsize_t multipleOfFour(vsize_t n);

// TEMP
free_header_t *get_header(vlink_t index) {
    return (free_header_t*) (memory + index);
}

vlink_t get_header_index(free_header_t *header) {
    return (byte*) header - (byte*) memory;
}
// END TEMP

// Input: size - number of bytes to make available to the allocator
// Output: none              
// Precondition: Size >= 1024
// Postcondition: `size` bytes are now available to the allocator
// 
// (If the allocator is already initialised, this function does nothing,
//  even if it was initialised with different size)

// Mostly complete !!
void vlad_init(u_int32_t size)
{
    // * size==0 might not be in the spec, need to check this
    if(memory!=NULL || size==0) return;

    size = powerOfTwo(size);

    memory = malloc(size);
    // if malloc fails, an error message is diplayed and the program will exit
    if(memory==NULL){
        fprintf(stderr, "vlad_init: Insufficient memory\n");
        exit(EXIT_FAILURE);
    }

    // set global variable values

    // initially zero - since first position in the memory[] array
    free_list_ptr = 0;
    memory_size = size;
    strategy = BEST_FIT;

    // setup the initial region header
    free_header_t *regionHeader = (free_header_t*)(memory+0);
    regionHeader -> magic = MAGIC_FREE;
    regionHeader -> size = size;
    // next and prev should point to the header itself
    regionHeader -> next = 0;
    regionHeader -> prev = 0;

}

// Input: n - number of bytes requested
// Output: p - a pointer, or NULL
// Precondition: n < size of largest available free block
// Postcondition: If a region of size n or greater cannot be found, p = NULL 
//                Else, p points to a location immediately after a header block
//                      for a newly-allocated region of some size >= 
//                      n + header size.
vsize_t max(vsize_t a, vsize_t b) {
    return (a>b)?a:b;
}

vsize_t highest_power(vsize_t num, vsize_t m) {
    int i;
    int highest = 0;
    int taken = 0;
    for (i = 0; i < sizeof(vsize_t) * 8 - 1; i++) {
        if (num & (1 << i)) {
            highest = i;
            taken++;
        }
    }
    return max(m, (1 << (highest + (taken > 1 ? 1 : 0))));
}
void *vlad_malloc(u_int32_t n)
{
    //int firstLoop = TRUE;
    //int result = FALSE;

    // round up n to the nearest multiple of four
    // if already a multiple of four, it will just return n
    //n = multipleOfFour(n);
    n = highest_power(n + sizeof(free_header_t), 16);

    
    free_header_t *curr = (free_header_t*)(memory + free_list_ptr);
    //free_header_t *smallest = curr;

    // transverse the free list 
    // store the current smallest chunk of memory
    // when a smaller chunk is found, replace it 
    // finish the loop when back to the start (free_list_ptr)
    do {
        if (curr->size < n) {
            curr = get_header(curr->next);
        } else {
            break;
        }
    } while (curr != get_header(free_list_ptr));
    
    if (curr->size < n) {
        return NULL;
    }

    // we have now found the smallest chunk of memory that can be used
    // we need to compare this to the threshold 
    // threshold = ALLOC_HEADER_SIZE + n +2*FREE_HEADER_SIZE
    // if our smallest value > threshold, we need to split the memory up
    // else just allocate the whole chunk 

    // still testing ! 
    
    //curr=smallest;

    /*
    if(curr->size >= (ALLOC_HEADER_SIZE + n + 2*FREE_HEADER_SIZE)){
        free_header_t *splitHeader = (free_header_t*)(((byte*) curr - (byte*) memory) + (curr->size + n));
        splitHeader->magic = MAGIC_FREE;
        splitHeader->size = (curr->size - n);
        splitHeader->prev = ((byte*) curr - (byte*) memory);
        splitHeader->next = curr->next;
    }
    */

// TEMP
    while (curr->size != n) {
        free_header_t *new_header = get_header(get_header_index(curr) + curr->size/2);
        new_header->magic = MAGIC_FREE;
        new_header->next = curr->next;
        new_header->prev = get_header_index(curr);
        new_header->size = curr->size / 2;
        
        curr->size /= 2;

        free_header_t *next = get_header(curr->next);
        next->prev = get_header_index(new_header);
        curr->next = get_header_index(new_header);
    }
// END TEMP

    if (get_header_index(curr) == curr->next) {
        return NULL;
    }

    free_header_t *prev = get_header(curr->prev);
    free_header_t *next = get_header(curr->next);

    prev->next = curr->next;
    next->prev = curr->prev;

    if (get_header_index(curr) == free_list_ptr) {
        free_list_ptr = get_header_index(next);
    }    
    
    curr->magic = MAGIC_ALLOC; 

    return ((byte*) curr + sizeof(free_header_t));
}


// Input: object, a pointer.
// Output: none
// Precondition: object points to a location immediately after a header block
//               within the allocator's memory.
// Postcondition: The region pointed to by object has been placed in the free
//                list, and merged with any adjacent free blocks; the memory
//                space can be re-allocated by vlad_malloc

void vlad_free(void *object)
{
    // TODO for Milestone 3
    vlad_merge();
}

// Input: current state of the memory[]
// Output: new state, where any adjacent blocks in the free list
//            have been combined into a single larger block; after this,
//            there should be no region in the free list whose next
//            reference is to a location just past the end of the region

static void vlad_merge()
{
    // TODO for Milestone 4
}

// Stop the allocator, so that it can be init'ed again:
// Precondition: allocator memory was once allocated by vlad_init()
// Postcondition: allocator is unusable until vlad_int() executed again

// Complete ! 
void vlad_end(void)
{
    free(memory);
    assert(memory==NULL);

    // update global variables
    memory_size=0;
}

// Precondition: allocator has been vlad_init()'d
// Postcondition: allocator stats displayed on stdout

void vlad_stats(void)
{
    // TODO
    // put whatever code you think will help you
    // understand Vlad's current state in this function
    // REMOVE all of these statements when your vlad_malloc() is done
    printf("vlad_stats() won't work until vlad_malloc() works\n");
    return;
}

// My functions - To make things easier

// returns the smallest power of two which is larger than the input size
// Complete ! 

static vsize_t powerOfTwo(vsize_t size){

    int input = size;
    int idealSize = 2;

    // if already a power of two, return the size
    // if not a power of two, break out of the loop
    while(input != 1){

        if(input % 2 != 0) break;
        input = input / 2;
    }

    // if input is 1, this means that it is already a power of two and can be returned
    if(input == 1) return size;
    
    // increment by power of two until idealSize is larger than size
    // this should find the smallest power of two that is larger than given size
    while(idealSize < size){
        idealSize = idealSize * 2;
    }

    return idealSize;
}
/*
static vsize_t multipleOfFour(vsize_t n){

    int input = n;

    if(input % 4 == 0 && input >= MIN_MEMORY){
        return input;
    }

    if(input < MIN_MEMORY){
        return MIN_MEMORY;
    }

    while(input % 4 != 0){
        input++;
    }

    return input;
}
*/