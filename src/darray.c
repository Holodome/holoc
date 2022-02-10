#include "darray.h"

#include <stddef.h>
#include <stdlib.h>

void *
da_grow(void *da, uintptr_t stride) {
    void *result;
    if (da) {
        darray_header *header = da_header(da);
        uint32_t new_capacity = header->capacity * 2;
        uintptr_t new_size    = sizeof(darray_header) + new_capacity * stride;

        header           = realloc(header, new_size);
        header->capacity = new_capacity;

        result = header + 1;
    } else {
        result = da_reserve_(stride, DARRAY_DEFAULT_SIZE);
    }

    return result;
}

void *
da_reserve_(uintptr_t stride, uint32_t count) {
    void *result = NULL;
    if (count) {
        uintptr_t size = sizeof(darray_header) + stride * count;

        darray_header *header = calloc(1, size);
        header->capacity      = count;

        result = header + 1;
    }

    return result;
}

void
da_free_(void *da, uintptr_t stride) {
    (void)stride;
    darray_header *header = da_header(da);
    free(header);
}
