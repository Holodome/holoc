#include "darray.h"

#include <stdlib.h>

#define DA_DEFUALT_SIZE 10

void *
da_reserve_(uint32_t stride, uint32_t count) {
    void *result = 0;
    if (count) {
        uint64_t initial_size = sizeof(darray_header) + count * stride;
        darray_header *header = calloc(initial_size, 1);
        header->capacity = count;
        result = header + 1;
    }
    
    return result;
    
}

void *
da_grow(void *a, uint32_t stride) {
    void *result = 0;
    if (a) {
        darray_header *header = da_header(a);
        uint64_t new_size = sizeof(*header) + header->capacity * stride * 2;
        header = realloc(header, new_size);
        header->capacity *= 2;
        result = header + 1;
    } else {
        result = da_reserve_(stride, DA_DEFUALT_SIZE);
    }
    return result;
}

void 
da_free_(void *a, uint32_t stride) {
    darray_header *header = da_header(a);
    free(header);    
}
