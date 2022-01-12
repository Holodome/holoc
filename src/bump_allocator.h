/*
Author: Holodome
Date: 11.12.2021
File: src/bump_allocator.h
Version: 0
*/
#ifndef BUMP_ALLOCATOR
#define BUMP_ALLOCATOR

#include "types.h"

struct allocator;

#define BUMP_DEFAULT_MINIMAL_BLOCK_SIZE ((1 << 12) - sizeof(bump_allocator_block))
#define BUMP_DEFAULT_ALIGN 16
// Structure holding data about single memory block
typedef struct bump_allocator_block {
    // Number of bytes used in block
    uint32_t used;
    // Total size of block data
    uint32_t size;
    // Data pointer
    uint8_t *data;
    // Linked list pointer
    struct bump_allocator_block *next;
} bump_allocator_block;

// Structure holding data about individual allocator, its state and settings
typedef struct bump_allocator {
    // Current block, linked list
    bump_allocator_block *block;
    // Number of temp allocations
    uint32_t temp_memory_count;
    // align of allocations 
    uint64_t align;
    // Minimal size of allocated block, to prevent fragmentation of system level
    uint64_t minimal_block_size;
    // Debug info, number of individual allocations
    uint32_t allocation_count;
    // Debug info, total number of bytes allocated
    uint64_t total_size;
} bump_allocator;

// Structure holding data about temporary memory record
typedef struct {
    // All this data is internal
    bump_allocator *allocator;
    bump_allocator_block *block;
    uint32_t block_used;
} temp_memory;

// Initializes bump allocator with given settings
// NOTE: This function may be omitted, then default settings are used
void bump_init(bump_allocator *allocator, uint64_t align, uint64_t minimal_block_size);
// Allocate size bytes on allocator
void *bump_alloc(bump_allocator *allocator, uint64_t size);
// Clears the allocator - frees all memory
void bump_clear(bump_allocator *allocator);
// Makes temporary memory start - all allocations will be freed after bump_temp_end call
temp_memory bump_temp(bump_allocator *allocator);
// End temporary memory
void bump_temp_end(temp_memory temp);
// Returns allocator object suitable for allocating using given allocator
struct allocator *bump_get_allocator(bump_allocator *a);

#endif 