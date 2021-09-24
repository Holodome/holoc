// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/memory.h
// Version: 0
//
// Defines memory-related functions, as well as block and arena allocator.
// Note that behaviour of allocator and idea behind it is different from standard libary
// All allocated memory is always implicitly set 0, (Zero Is Initialization - unlike RAII)
// Defining own memory allocation functions is useful in debugging - 
// virtual memory-based bounds checking can be implicitly done in debugger with use of 
// os protection functions (ex. VirtualProtect on windows)
// 
// Idea behind ZII is to provide general way of allocating objects. Thus it can be said 
// that any (besides things that don't) object can be created by setting it to zero.
// ex. MemoryArena arena = {0}; // Arena can be used from now, without explicit initializtion (arena = create_arena(...))
//
// @NOTE MemoryArena is not a replacement for general purpose allocator.
// It is just simplification for many cases in programming compilers, where lifetime of a lot
// of small object can be assigned to a liftime of one greater. This is what rust has better explicity doing with 'a
#pragma once 
#include "general.h"

// standard library-like functions
// Unlike malloc, mem_alloc is guaranteed to return already zeroed memory
// malloc
void *mem_alloc(uptr size);
// realloc
void *mem_realloc(void *ptr, uptr old_size, uptr size);
// strdup
char *mem_alloc_str(char *str);
// free
void mem_free(void *ptr, uptr size);
// memcpy
void mem_copy(void *dst, const void *src, uptr size);
// memmove
void mem_move(void *dst, const void *src, uptr size);
// memset
void mem_zero(void *dst, uptr size);

// Memory blocks are used in arena-like allocators
// Used to batch several allocations together instead of making multiple os callss
typedef struct MemoryBlock { 
    uptr size; 
    uptr used; 
    u8 *base;
    // Used only in arena, not related to block itselfs
    struct MemoryBlock *next;
} MemoryBlock;

MemoryBlock *mem_alloc_block(uptr size);
// Blocks can be freed with mem_free call

#define MEMORY_ARENA_DEFAULT_MINIMAL_BLOCK_SIZE MB(4)
#define MEMORY_ARENA_DEFAULT_ALIGNMENT 16

// Region-based memory management.
// In most cases, memory allocation lifetime can be logically assigned to lifetime
// of some parent object. For example, all tokens exist within tokenizer, AST exists 
// within parser etc. 
// Because of that, life of programmer can be made easier by using arena allocators.
// By allocating on arena, freeing all memory can be done in single call, without need
// to manually free each object. Besides that, arena allocator allocates big blocks of 
// memory in advance, which lowers number of OS calls, which makes memory consumption of 
// single allocation more stable (which ofthen becomes bottleneck in other cases).  
//
// Structure used to link several allocations together as well as allocate from blocks
// Can be zero-initialized, then minimal_block_size is set to default value
// How arena manages its blocks is not controlled by other parts of this program
// Calling arena-based allocation functions makes the most optimal allocation in terms of saving memory
// and user has no control over that.
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
// Allocates structure using arena that is located inside this structure
// Ex: struct A { MemoryArena a; }; struct A *b = arena_bootstrap(A, a); (b is allocated on b.a arena)
#define arena_bootstrap(_type, _field) arena_bootstrap_(sizeof(_type), STRUCT_OFFSET(_type, _field))
void *arena_bootstrap_(uptr size, uptr arena_offset);

// Alloctions guarded by temporary memory calls are not commited to arena
typedef struct TemporaryMemory {
    MemoryArena *arena;
    MemoryBlock *block;
    uptr block_used;
} TemporaryMemory;

TemporaryMemory begin_temp_memory(MemoryArena *arena);
void end_temp_memory(TemporaryMemory temp);

// @NOTE there are different allocation technicues (memory pool, packed set, slab allocators, free list linked lists)
// that program may want to use later.
// Because what allocation may seem suitable, we can't predict all use cases and features it should support
// But in later future, it may be benefitial to have some kind of allocator object - such that usage code
// may not specify allocation technique (like call arena_alloc - specific function). This is substaintilly better 
// for prototyping

// Temporary allocation. The way it works is by having allocator designated for 
// temporary allocations. This is common idiom in games, where temporary allocator can free all memory within frames
// Here it should be used primarily for prototyping, as its execution time is not constant (due to need to zero memory) 
// When clear is set to true, all allocations overwrite previous ones.
// When clear is not zero, allocations pile up 
void tmp_alloc_set_clear(b32 clear);
void *tmp_alloc(uptr size);