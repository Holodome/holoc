#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "types.h"

// Signature of the realloc function used in allocator
#define ALLOCATOR_REALLOC(_name) \
    void *_name(void *internal, void *ptr, uint64_t old_size, uint64_t new_size)
// Reallocation function type
typedef ALLOCATOR_REALLOC(allocator_realloc);

// Allocator is a structure that wraps functionality of allocating memory with
// different algorithms This is useful in writing memory-allocating algorithms
// without specifying the memory allocating policy instead allowing to write
// more generic code. The way individual allocator allocates memory is
// completely up to implementation, but it must always return some memory on
// request with non-null new_size
typedef struct allocator {
    // Implementation-defined state data
    void *internal;
    // Function used to allocate memory
    allocator_realloc *realloc;
} allocator;

// malloc wrapper
#define aalloc(_a, _size) (_a)->realloc((_a)->internal, 0, 0, (_size))
// free
// Note that size must be passed too
#define afree(_a, _data, _size) (_a)->realloc((_a), (_data), (_size), 0)
// realloc
// Note that old size must be passed too
#define arealloc(_a, _data, _old_size, _new_size) \
    (_a)->realloc((_a), (_data), (_old_size), (_new_size))

// Returns allocator that uses libc functions (malloc, free, realloc)
allocator *get_system_allocator(void);

#endif
