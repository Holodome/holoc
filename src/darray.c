#include "darray.h"

#include <stddef.h>

#include "allocator.h"

void *
da_grow(void *da, uintptr_t stride, allocator *a) {
    void *result;
    if (da) {
        darray_header *header = da_header(da);
        uint32_t new_capacity = header->capacity * 2;
        uintptr_t old_size = sizeof(darray_header) + header->capacity * stride;
        uintptr_t new_size = sizeof(darray_header) + new_capacity * stride;

        header           = arealloc(a, header, old_size, new_size);
        header->capacity = new_capacity;

        result = header + 1;
    } else {
        result = da_reserve_(stride, DARRAY_DEFAULT_SIZE, a);
    }

    return result;
}

void *
da_reserve_(uintptr_t stride, uint32_t count, allocator *a) {
    void *result = NULL;
    if (count) {
        uintptr_t size = sizeof(darray_header) + stride * count;

        darray_header *header = aalloc(a, size);
        header->capacity      = count;

        result = header + 1;
    }

    return result;
}

void
da_free_(void *da, uintptr_t stride, allocator *a) {
    darray_header *header = da_header(da);
    afree(a, header, header->capacity * stride);
}
