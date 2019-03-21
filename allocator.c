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

ALL COMPLETE !!!! 

** To do ** 
1) Implement vlad_init() & vlad_end() - Will need highest power calculator function 
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
#define MIN_MEMORY 16
#define TRUE 1
#define FALSE 0
#define THRESHOLD (n + 2*FREE_HEADER_SIZE)

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
static vsize_t multipleOfFour(vsize_t n);
static void *makeRealPtr(vaddr_t ptr);
static vaddr_t makeOffsetPtr(void *ptr);
static void checkHeader(void *ptr);
static u_int32_t adjacent(free_header_t *ptr);

// Input: size - number of bytes to make available to the allocator
// Output: none              
// Precondition: Size >= 1024
// Postcondition: `size` bytes are now available to the allocator
// 
// (If the allocator is already initialised, this function does nothing,
//  even if it was initialised with different size)

// ** Complete ** 
void vlad_init(u_int32_t size)
{
    if(memory!=NULL) return;

    size = powerOfTwo(size);

    memory = malloc(size);
    // if malloc fails, an error message is diplayed and the program will exit
    if(memory==NULL){
        fprintf(stderr, "vlad_init: Insufficient memory\n");
        exit(EXIT_FAILURE);
    }
    
    // set global variable values
    free_list_ptr = 0;
    memory_size = size;
    strategy = BEST_FIT;

    // setup the initial region header
    free_header_t *regionHeader = makeRealPtr(free_list_ptr);
    regionHeader->magic = MAGIC_FREE;
    regionHeader->size = size;
    // next and prev should point to the header itself
    regionHeader->next = 0;
    regionHeader->prev = 0;
}

// Input: n - number of bytes requested
// Output: p - a pointer, or NULL
// Precondition: n < size of largest available free block
// Postcondition: If a region of size n or greater cannot be found, p = NULL 
//                Else, p points to a location immediately after a header block
//                      for a newly-allocated region of some size >= 
//                      n + header size.

void *vlad_malloc(u_int32_t n)
{
    int firstLoop = TRUE;
    int result = FALSE;
    int numCount=0;

    // round up n to the nearest multiple of four
    // if already a multiple of four, it will just return n
    n = multipleOfFour(n + ALLOC_HEADER_SIZE);

    //printf("%d\n",n);
    //printf("%d\n",memory_size);
    
    free_header_t *curr = makeRealPtr(free_list_ptr);
    free_header_t *smallest = curr;

    // transverse the free list 
    // store the current smallest chunk of memory
    // when a smaller chunk is found, replace it 
    // finish the loop when back to the start (free_list_ptr)
    while(curr != makeRealPtr(free_list_ptr) || firstLoop==TRUE){
        if(curr->size >= n){
            if(firstLoop){
                smallest = curr;
                firstLoop=FALSE;
            } else {
                if(smallest->size > curr->size){
                    smallest = curr;
                }
            } 
            result=TRUE;
        }
        curr = makeRealPtr(curr->next);
        if(curr == makeRealPtr(free_list_ptr)){
            firstLoop = FALSE;
        }
        numCount++;
    }
    curr=smallest;
    
    // if there is no chunk of memory to fit n, return NULL immediately
    // result will be true if there is a chunk of memory to fit n
    if(result == FALSE){
        return NULL;
    }

    // we have now found the smallest chunk of memory that can be used
    // we need to compare this to the threshold 
    // if our smallest value > threshold, we need to split the memory up
    // else just allocate the whole chunk - unless it is the last free chunk available
    if(curr->size >= THRESHOLD){

        free_header_t *freeHeader = makeRealPtr(makeOffsetPtr(curr) + n);
        freeHeader->magic = MAGIC_FREE;
        freeHeader->size = curr->size - n;
        freeHeader->next = curr->next;
        freeHeader->prev = makeOffsetPtr(curr);

        curr->size = n;

        // connect freeHeader with the rest of the free list
        free_header_t *next = makeRealPtr(curr->next);
        next->prev = makeOffsetPtr(freeHeader);
        curr->next = makeOffsetPtr(freeHeader);

    } else if(numCount == 1){
        return NULL;
    }

    // covers both cases 
    // 1) smaller than threshold and enough free regions - simply remove curr
    // 2) larger than threshold and we need to remove curr from the free list
    free_header_t *prev = makeRealPtr(curr->prev);
    free_header_t *next = makeRealPtr(curr->next);
    prev->next = curr->next;
    next->prev = curr->prev;        
    
    // free list pointer update 
    if(curr == makeRealPtr(free_list_ptr)){
        free_list_ptr = curr->next;
    }

    // check the header to ensure no arbitrary numbers
    checkHeader(curr);
    curr->magic = MAGIC_ALLOC;  

    return ((void*) curr + ALLOC_HEADER_SIZE);
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
    // make sure that the region is valid
    // print an error message and return if not a valid region
    // otherwise, make the allocated region header into a free header

    free_header_t *freePtr = (free_header_t*) ((void*) object - ALLOC_HEADER_SIZE);
    int objectPos = makeOffsetPtr(object);

    if(objectPos < 0 || objectPos > memory_size){
        fprintf(stderr, "vlad_free: Attempt to free via invalid pointer\n");
        exit(EXIT_FAILURE);
    }

    if(freePtr->magic != MAGIC_ALLOC){
        fprintf(stderr, "vlad_free: Attempt to free non-allocated memory\n");
        exit(EXIT_FAILURE);
    } else {
        checkHeader(freePtr);
        freePtr->magic = MAGIC_FREE;
    }

    int firstLoop = TRUE;
    free_header_t *curr = makeRealPtr(free_list_ptr);

    while(curr != makeRealPtr(free_list_ptr) || firstLoop == TRUE){
        if(freePtr->size >= curr->size){
            if(curr->next == free_list_ptr){
                curr = makeRealPtr(curr->next);
                break;
            }
            curr = makeRealPtr(curr->next);
        } else {
            break;
        }
    }

    // update free_list_ptr to point to first free region
    free_header_t *temp = makeRealPtr(free_list_ptr);
    if(freePtr->size < temp->size){
        free_list_ptr = makeOffsetPtr(freePtr);
    } 

    free_header_t *prev = makeRealPtr(curr->prev);
    prev->next = makeOffsetPtr(freePtr);
    freePtr->prev = makeOffsetPtr(prev);

    curr->prev = makeOffsetPtr(freePtr);
    freePtr->next = makeOffsetPtr(curr);
    
    vlad_merge();
}

// Input: current state of the memory[]
// Output: new state, where any adjacent blocks in the free list
//            have been combined into a single larger block; after this,
//            there should be no region in the free list whose next
//            reference is to a location just past the end of the region

static void vlad_merge()
{
    free_header_t *curr = makeRealPtr(free_list_ptr);
    int firstLoop = TRUE;
    int i = 0;

    while(i < 2){

        firstLoop = TRUE;
        curr = makeRealPtr(free_list_ptr);
        while(curr != makeRealPtr(free_list_ptr) || firstLoop == TRUE){
            if(adjacent(curr)){
                free_header_t *prev = makeRealPtr(curr->prev);
                free_header_t *next = makeRealPtr(curr->next);
                free_header_t *nextRegion = (free_header_t*) ((void*)(curr) + curr->size);
                free_header_t *temp = nextRegion;

                curr->size += nextRegion->size;
                next = makeRealPtr(nextRegion->next);
                prev = makeRealPtr(nextRegion->prev);

                prev->next = makeOffsetPtr(next);
                next->prev = makeOffsetPtr(prev);
                
                firstLoop = FALSE;

                temp->magic = 0;
                temp->size = 0;
                temp->next = 0;
                temp->prev = 0;

                // time to go through memory and find the first free region
                // update free_list_ptr
                
                curr = makeRealPtr(0);
                do{
                    if(curr->magic != MAGIC_FREE){
                        curr = makeRealPtr(makeOffsetPtr(curr) + curr->size);
                    } else {
                        break;
                    }
                } while(curr != makeRealPtr(4096));
                
                free_list_ptr = makeOffsetPtr(curr);

            } else if(curr->next == free_list_ptr){
                break;
            } 
            curr = makeRealPtr(curr->next);
        }
        i++;
    }
}

// ** Complete **
static u_int32_t adjacent(free_header_t *ptr){

    int result = TRUE;

    free_header_t *nextRegion = (free_header_t*) ((void*)(ptr) + ptr->size);

    // if at the end of list, wrap back around to beginning
    if(makeOffsetPtr(nextRegion) == 4096){
        nextRegion = makeRealPtr(0);
        result = FALSE;
    }
    if(nextRegion->magic != MAGIC_FREE){
        return FALSE;
    }
    return result;
}

// Stop the allocator, so that it can be init'ed again:
// Precondition: allocator memory was once allocated by vlad_init()
// Postcondition: allocator is unusable until vlad_int() executed again

// ** Complete **
void vlad_end(void)
{
    if(memory == NULL) return;

    free(memory);
    memory = NULL;
}

// Precondition: allocator has been vlad_init()'d
// Postcondition: allocator stats displayed on stdout

// ** Complete **
    void vlad_stats(void)
{
	// TODO
	// put whatever code you think will help you
	// understand Vlad's current state in this function
	// REMOVE all of these statements when your vlad_malloc() is done
	//printf("vlad_stats() won't work until vlad_malloc() works\n");

    // I used this to the current state of memory
    // I have been given permission by Tony Bao to use this code
    // all credits go to him 

    printf("** Printing Memory **\n ");
	printf("Block starts @ %p\n", memory);

	byte * cpAddress = memory;

	int i = 0;
	while (i < 2000){
		if (i % 10 == 0 && i != 0) printf (" == %d\n", i);
		printf ("[%-3u] - ", *cpAddress);
		cpAddress += 1; i ++;
	}
	printf("\n");
	return;
}

// My functions - To make things easier

// returns the smallest power of two which is larger than the input size

// ** Complete **
static vsize_t powerOfTwo(vsize_t size){

    int input = size;
    int idealSize = 2;
    int min = 1024;

    // round up to 1024 nbytes if given a smaller value
    if(size <= min){
        return min;
    }

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

// returns the smallest power of four which is larger or equal to n

// ** Complete **
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

// To convert a vaddr_t value to a real C pointer
// Add the vaddr_t value to &memory[0] and then type cast it to (void *).

// ** Complete **
static void *makeRealPtr(vaddr_t ptr){

    return (void *)(memory + ptr);
}

// To convert a real C pointer to a vaddr_t value
// Compute the difference between the pointer and &memory[0], and then type cast it to vaddr_t

// ** Complete **
static vaddr_t makeOffsetPtr(void *ptr){

    return ( (void*) ptr - (void*) memory );
}

// check that the arbitrary number in the header is correct
// If not, terminate the program after printing an appropriate error message

// ** Complete **
static void checkHeader(void *ptr){

    free_header_t *temp = makeRealPtr(makeOffsetPtr(ptr));
    if(temp->magic == MAGIC_ALLOC || temp->magic == MAGIC_FREE){
        return;
    } else {
        fprintf(stderr, "vald_alloc: Memory corruption\n");
        exit(EXIT_FAILURE);
    }
}
