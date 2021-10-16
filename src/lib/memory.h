// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/lib/memory.h
// Version: 0
//
// Defines memory-related functions, as well as block and arena allocator.
// @NOTE All allocation functions set allocated memory to zero
#pragma once 
#include "lib/general.h"
#include "lib/utils.h"

#define MEM_USE_STDLIB 0

#define MEM_ALLOC_UNDERFLOW_CHECK 0x1
#define MEM_ALLOC_OVERFLOW_CHECK  0x2

// @NOTE: current memory system allows dynamically created and growing arenas
// Other option is to preallocate memory for whole game and allocate from it 
// selectively using subarens (for example let game use 8G, 4G be assets, 2G game world etc.)
// This is actually way more faster and effective and dynamic arenas
// Dynamic arenas have some problems like memory fragmentation, but allow not to care about arena sizes
// and tweak only default size
// In end of development we may want to switch to preallocating memory for whole game to be more
// effective and fast and let user specify amounth of memory that game can allocate
// @NOTE: Also using individual arenas can help catch memory access bugs, like overflow or underflow
#define MEM_DEFUALT_ARENA_BLOCK_SIZE MB(2)
#define MEM_DEFAULT_ALIGNMENT 16
CT_ASSERT(IS_POW2(MEM_DEFAULT_ALIGNMENT));
#define MEM_DEFAULT_ALLOC_FLAGS 0
// @NOTE: DEBUG STUFF
// Flag if all block allocations should have guarded pages around them
#define MEM_DO_BOUNDS_CHECKING 0
#define MEM_BOUNDS_CHECKING_POLICY MEM_ALLOC_OVERFLOW_CHECK
// Flag if all allocations are done in separate blocks each of which is guarded
// This is highly ineffective and it is even not certain that we will be able to do so 
// with a lot of allocations because we could simply run out of memory
// Basically only this option enables us to do full bounds check, because bounds only for blocks
// may not be able to catch a lot of memory access errors within blocks, and even allow overflows
// up to alignment size
// @TODO: there is a way to reliably do hard bounds checking selectively if we pass this down as a 
// flag and don't record hard bounds-checked allocation in debug system
// We may want to switch having this as an option in contrast to global flag if running out of 
// memory becomes a problem
// But really without hard checks bounds checking makes little sence
#define MEM_DO_HARD_BOUNDS_CHECKING_INTERNAL 1
#define MEM_DO_HARD_BOUNDS_CHECKING (MEM_DO_HARD_BOUNDS_CHECKING_INTERNAL && MEM_DO_BOUNDS_CHECKING)
CT_ASSERT((u32)MEM_DO_HARD_BOUNDS_CHECKING <= MEM_DO_BOUNDS_CHECKING);

#if MEM_DO_BOUNDS_CHECKING
// @NOTE: There is no way to do both overflow and underflow check simulateously due
// to page size limitations.
// We don't raise errors in platform layer but do it here
CT_ASSERT((u32)TO_BOOL(MEM_BOUNDS_CHECKING_POLICY & MEM_ALLOC_OVERFLOW_CHECK) + (u32)TO_BOOL(MEM_BOUNDS_CHECKING_POLICY & MEM_ALLOC_UNDERFLOW_CHECK) == 1);
#undef MEM_DEFAULT_ALLOC_FLAGS
#define MEM_DEFAULT_ALLOC_FLAGS MEM_BOUNDS_CHECKING_POLICY
#endif 

#if MEM_DO_HARD_BOUNDS_CHECKING
#undef MEM_DEFAULT_ALIGNMENT
#define MEM_DEFAULT_ALIGNMENT 1
#endif 

// standard library-like functions
// Unlike malloc, mem_alloc is guaranteed to return already zeroed memory
// malloc
ATTR((malloc))
void *mem_alloc(uintptr_t size);
// realloc
void *mem_realloc(void *ptr, uintptr_t old_size, uintptr_t size);
// strdup
char *mem_alloc_str(const char *str);
// free
void mem_free(void *ptr, uintptr_t size);
// memcpy
void mem_copy(void *dst, const void *src, uintptr_t size);
// memmove
void mem_move(void *dst, const void *src, uintptr_t size);
// memset
void mem_zero(void *dst, uintptr_t size);
// memcmp
bool mem_eq(const void *a, const void *b, uintptr_t n);

// Memory blocks are used in arena-like allocators
// Used to batch several allocations together instead of making multiple os callss
typedef struct Memory_Block { 
    uintptr_t size; 
    uintptr_t used; 
    u8 *base;
    // Used only in arena, not related to block itselfs
    struct Memory_Block *next;
} Memory_Block;

ATTR((malloc))
Memory_Block *mem_alloc_block(uintptr_t size);
void mem_free_block(Memory_Block *block);
// Blocks can be freed with mem_free call

// Region-based memory management.
// In most cases, memory allocation lifetime can be logically assigned to lifetime
// of some parent object. For example, all tokens exist within lexer, AST exists 
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
// and user has no control over that
//
// @NOTE(hl): One of the latter benefits of memory arenas is inside the multithreaded code
typedef struct Memory_Arena {
    Memory_Block *current_block;
    uintptr_t minimum_block_size;
    int temp_memory_count;
} Memory_Arena;

// Allocator functions
// All arena-based alloctions are aligned according to MEMORY_ARENA_DEFAULT_ALIGNMENT
#define arena_alloc_struct(_arena, _type) (_type *)arena_alloc(_arena, sizeof(_type))
#define arena_alloc_array(_arena, _count, _type) (_type *)arena_alloc(_arena, sizeof(_type) * _count)
void *arena_alloc(Memory_Arena *arena, uintptr_t size);
char *arena_alloc_str(Memory_Arena *arena, const char *src);
void *arena_copy(Memory_Arena *arena, const void *src, uintptr_t size);
void arena_free_last_block(Memory_Arena *arena);
// Frees all blocks
void arena_clear(Memory_Arena *arena);
// Allocates structure using arena that is located inside this structure
// Ex: struct A { Memory_Arena *a; }; struct A *b = arena_bootstrap(A, a); (b is allocated on b.a arena)
#define arena_bootstrap(_type, _field) (_type *)arena_bootstrap_(sizeof(_type), STRUCT_OFFSET(_type, _field))
void *arena_bootstrap_(uintptr_t size, uintptr_t arena_offset);

// Alloctions guarded by temporary memory calls are not commited to arena
typedef struct TemporaryMemory {
    Memory_Arena *arena;
    Memory_Block *block;
    uintptr_t block_used;
} TemporaryMemory;

TemporaryMemory begin_temp_memory(Memory_Arena *arena);
void end_temp_memory(TemporaryMemory temp);
