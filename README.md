# Custom Dynamic Memory Allocator in C

A user-space heap allocator implemented in C using `mmap`, supporting
`malloc` and `free` with a linked-list free list and first-fit
allocation strategy.

## Overview

This project implements a simplified dynamic memory allocator to
understand how heap management works internally.

Key features:

-   Memory pool backed by `mmap`
-   Linked-list free list
-   First-fit allocation strategy
-   Block splitting for efficient reuse
-   Adjacent block coalescing to reduce fragmentation
-   8-byte memory alignment
-   In-memory block metadata management

## Architecture

Each block stores metadata in-band:

    typedef struct Block {
        size_t size;
        int is_free;
        struct Block* next;
    } Block;

Allocation flow: 
1. Align requested size to 8 bytes\
2. Search free list (first-fit)\
3. Split block if oversized\
4. Return pointer past metadata

Free flow: 
1. Mark block as free\
2. Coalesce with adjacent free block

## Build & Run

    gcc allocator.c -o allocator
    ./allocator

## Concepts Demonstrated

-   Virtual memory management (`mmap`)
-   Manual heap management
-   Fragmentation control
-   Pointer arithmetic
-   Memory alignment
-   Low-level systems programming

## Future Improvements

-   Implement `realloc` / `calloc`
-   Add boundary tags for bidirectional coalescing
-   Add benchmarking vs glibc `malloc`
-   Add thread safety
