#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

/*=================================== APPROACH ================================================

    1. Define the memory pool from the OS for allocation/deletion purposes.
    2. Go through each block of this pool and see which is free or not.
    3. Make a 'free' list of these blocks and track them accordingly.

==============================================================================================*/


/*=============================================================================================
    STEP 1. Defining the block header structure:
        - Tracks the size
        - Checks whether the block is free or not
        - Pointer to the next free block (for free list)
==============================================================================================*/

typedef struct Block{
    size_t size; // tracks the size
    int is_Free; // checks whether the block is free or not
    struct Block* next; // pointer to the next free block
} Block;

// Calculate the size of our metadata
#define BLOCK_HEADER_SIZE sizeof(Block)

/*=============================================================================================
    STEP 2. Defining the global variables:
        - Tracks the head of this free list
        - Checks the start of our memory pool (i.e. chunk of the memory that allocator grabs 
    from the OS)
        - Total size of the pool
==============================================================================================*/

static Block* free_list_head = NULL; //tracks the head of this free list
static void* memory_pool = NULL; //checks the start of our memory pool
static size_t pool_size = 0; //total pool size

/*=============================================================================================
    STEP 3. Initialize the allocator:
        - Request a large chunk of memory from the OS using mmap()
        - Set up the initial free block that spans the entire pool.
==============================================================================================*/

void allocator_init(size_t size){
    // Request memory from the OS using mmap
    // MAP_ANONYMOUS means not backed by a file
    // MAP_PRIVATE means changes are not shared
    memory_pool = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory_pool == MAP_FAILED){
        printf("Failed to allocate memory pool\n");
        return;
    }

    pool_size = size;

    // Initialize the first free block to span the entire pool

    free_list_head = (Block*)memory_pool;
    free_list_head->size = size - BLOCK_HEADER_SIZE;
    free_list_head->is_Free = 1;
    free_list_head->next = NULL;

    printf("Allocator initialized with %zu bytes\n", size);
}

/*=============================================================================================
    STEP 4. Align size:
        - Memory should be aligned to word boundaries for performance.
        - This function rounds up to the nearest multiple of 8.
==============================================================================================*/

size_t align_size(size_t size) {
    return (size + 7) & ~7;  // Round up to multiple of 8
}

/*=============================================================================================
    STEP 5. Split Block:
    If a block we find is larger than we needed ->
        - First part becomes the allocated block that we need.
        - Remainder stays as a smaller, free block.
==============================================================================================*/

void split_block(Block* block, size_t size){
    // Only split if there's enough room for a new block header + some data
    if(block->size >= size + BLOCK_HEADER_SIZE + 8){
        // Create a new block in the remaining space
        Block* new_block = (Block*)((char*)block + BLOCK_HEADER_SIZE + size);
        new_block->size = block->size - size - BLOCK_HEADER_SIZE;
        new_block->next = block->next;
        new_block->is_Free = 1;

        // Update the current block
        block->size = size;
        block->next = new_block;
    }
}