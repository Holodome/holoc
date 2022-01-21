#include "allocator.h"

#include <stdlib.h>
#include <string.h>

#include "bump_allocator.h"

static uint64_t allocated;

static ALLOCATOR_REALLOC(system_realloc) {
    void *new_ptr = 0;
    if (new_size == 0) {
        free(ptr);
    } else {
        new_ptr = realloc(ptr, new_size);
        if (new_size > old_size) {
            memset((char *)new_ptr + old_size, 0, new_size - old_size);
        }
    }
    allocated -= old_size;
    allocated += new_size;
    return new_ptr;
}

static allocator system_allocator = {0, system_realloc};

allocator *
get_system_allocator(void) {
    return &system_allocator;
}

#if HOLOC_DEBUG

allocator *
get_debug_allocator(void) {
    static bump_allocator a = {0};
    static allocator storage;
    storage = bump_get_allocator(&a);
    return &storage;
}

#endif
