#pragma once 
#include "general.h"

// Wrappers for stdandard functions
// Unlike malloc, mem_alloc is guaranteed to return already zeroed memory
// using mem_alloc instead of arena allocators is not advised in general use-cases
// malloc
void *mem_alloc(uptr size);
// strdup
char *mem_alloc_str(char *str);
// free
void mem_free(void *ptr);
// memcpy
void mem_copy(void *dst, const void *src, uptr size);
// memmove
void mem_move(void *dst, const void *src, uptr size);
// memset
void mem_zero(void *dst, uptr size);

// Memory blocks are used in arena-like allocators
// Instead of callyng system allocation procedure each time allocation needs to be made, 
// the program allocates blocks of memory in advance and allocates from them later
// Although this potentially wastes some space, usually this is not the issue
typedef struct MemoryBlock { 
    uptr size; 
    uptr used; 
    u8 *base;
    struct MemoryBlock *next;
} MemoryBlock;

MemoryBlock *mem_alloc_block(uptr size);
// Blocks can be freed with mem_free call

#define MEMORY_ARENA_DEFAULT_MINIMAL_BLOCK_SIZE MB(4)
#define MEMORY_ARENA_DEFAULT_ALIGNMENT 16
// Structure used to link several allocations together as well as allocate from blocks
// Can be zero-initialized, then minimal_block_size is set to default value
// How arena manages its blocks is not controlled by other parts of this program
// Calling arena-based allocation functions makes the most optimal allocation in terms of saving memory
typedef struct MemoryArena {
    MemoryBlock *current_block;
    uptr minimum_block_size;
    int temp_memory_count;
} MemoryArena;

// Allocator functions
// All arena-based alloctions are aligned according to MEMORY_ARENA_DEFAULT_ALIGNMENT
#define arena_alloc_struct(_arena, _type) (_type *)arena_alloc(_arena, sizeof(_type))
#define arena_alloc_array(_arena, _count, _type) (_type *)arena_alloc(_arena, sizeof(_type) * _count)
void *arena_alloc(MemoryArena *arena, uptr size);
char *arena_alloc_str(MemoryArena *arena, const char *src);
void *arena_copy(MemoryArena *arena, const void *src, uptr size);
void arena_free_last_block(MemoryArena *arena);
// Frees all blocks
void arena_clear(MemoryArena *arena);

// Alloctions guarded by temporary memory calls are not commited to arena
typedef struct TemporaryMemory {
    MemoryArena *arena;
    MemoryBlock *block;
    uptr block_used;
} TemporaryMemory;

TemporaryMemory begin_temp_memory(MemoryArena *arena);
void end_temp_memory(TemporaryMemory temp);
