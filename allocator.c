#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

/*=================================== APPROACH ================================================

    1. Define the memory pool from the OS for allocation/deletion purposes.
    2. Go through each block of this pool and see which is free or not.
    3. Make a 'free' list of these blocks and track them accordingly.

==============================================================================================*/

/*=================================== STRATEGY ================================================

    1. Keep a linked list of free memory blocks.
    2. When allocating, find the first block big enough.
    3. When freeing, return the block to the free list.

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

/*=============================================================================================
    STEP 6. Allocating the memory:
        - Search the free list for a block big enough using first-fit strategy.
        - Split the block if it's too large, then return pointer to usable memory.
==============================================================================================*/

void* my_malloc(size_t size){
    if(size == 0){
        return NULL;
    }
    // Align the size
    size = align_size(size);

    Block* current = free_list_head;
    Block* prev = NULL;
    while(current != NULL){
        if(current->is_Free && current->size >= size)
        {
            // A suitable block is found!

            // Split if the block requested is larger than expected
            
            split_block(current, size); 

            // Current block is occupied

            current->is_Free = 0;

            return (void*)((char*)current + BLOCK_HEADER_SIZE);
        }

        prev = current;
        current = current->next;

    }

    printf("Out of memory: couldn't allocate %zu bytes!\n", size);

}

/*============================================================================================
    STEP 7. Free the memory:
        - Mark the block as free and add it back to the free list.
        - Try to merge with adjacent free blocks to reduce fragmentation.
==============================================================================================*/

void my_free(void* ptr){
    if(ptr == NULL)
    {
        return;
    }

    // Get the header of the block
    Block* block = (Block*)((char*)ptr - BLOCK_HEADER_SIZE);

    // Free the block
    block->is_Free = 1;

    // Coalescing - Mergining two adjacent free spaces

    if(block->next && block->next->is_Free)
    {
        block->size = BLOCK_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
    printf("Freed %zu bytes\n", block->size);

}

/*============================================================================================
    STEP 8. Utility functions:
        - Helper functions to visualize and debug the allocator.
==============================================================================================*/

void print_memory_map() {
    printf("\n=== Memory Map ===\n");
    Block* current = free_list_head;
    int block_num = 0;
    
    while (current != NULL) {
        printf("Block %d: size=%zu, %s\n", 
               block_num++, 
               current->size,
               current->is_Free ? "FREE" : "ALLOCATED");
        current = current->next;
    }
    printf("==================\n\n");
}

void allocator_destroy() {
    if (memory_pool != NULL) {
        munmap(memory_pool, pool_size);
        memory_pool = NULL;
        free_list_head = NULL;
        pool_size = 0;
        printf("Allocator destroyed\n");
    }
}

/* ==========================================================================================
    STEP 9. Testing the Allocator:
============================================================================================= */

int main() {
    printf("Building a Custom Memory Allocator\n");
    printf("===================================\n\n");
    
    // Initialize with 1KB of memory
    allocator_init(1024);
    print_memory_map();
    
    // Test 1: Allocate some memory
    printf("Test 1: Allocating 100 bytes\n");
    void* ptr1 = my_malloc(100);
    if (ptr1) {
        strcpy((char*)ptr1, "Hello from custom allocator!");
        printf("Stored: %s\n", (char*)ptr1);
    }
    print_memory_map();
    
    // Test 2: Allocate more memory
    printf("Test 2: Allocating 200 bytes\n");
    void* ptr2 = my_malloc(200);
    print_memory_map();
    
    // Test 3: Free the first block
    printf("Test 3: Freeing first allocation\n");
    my_free(ptr1);
    print_memory_map();
    
    // Test 4: Allocate again (should reuse freed space)
    printf("Test 4: Allocating 50 bytes (should reuse freed space)\n");
    void* ptr3 = my_malloc(50);
    print_memory_map();
    
    // Test 5: Free everything
    printf("Test 5: Freeing all allocations\n");
    my_free(ptr2);
    my_free(ptr3);
    print_memory_map();
    
    // Cleanup
    allocator_destroy();
    
    return 0;
}