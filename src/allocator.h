#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "types.h"

#define ALLOCATOR_REALLOC(_name) void *_name(void *internal, void *ptr, uint64_t old_size, uint64_t new_size)
typedef ALLOCATOR_REALLOC(allocator_realloc);

typedef struct allocator {
    void *internal;
    allocator_realloc *realloc; 
} allocator;

#define aalloc(_a, _size) (_a)->realloc((_a), 0, 0, (_size))
#define afree(_a, _data, _size)  (_a)->realloc((_a), (_data), (_size), 0)
#define arealloc(_a, _data, _old_size, _new_size) (_a)->realloc((_a), (_data), (_old_size), (_new_size))

allocator *get_system_allocator(void);

#endif 
