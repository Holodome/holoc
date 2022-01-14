#include "allocator.h"

#include <stdlib.h>
#include <string.h>

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
    return new_ptr;
}

static allocator system_allocator = {0, system_realloc};

allocator *
get_system_allocator(void) {
    return &system_allocator;
}
