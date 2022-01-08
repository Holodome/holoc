/*
Author: Holodome
Date: 06.12.2021
File: src/darray.h
Version: 0
*/
#ifndef DARRAY_H
#define DARRAY_H

#include <stdint.h>

typedef struct {
    uint32_t size;
    uint32_t capacity;
} darray_header;

#define da_header(_da) ((darray_header *)((char *)(_da) - sizeof(darray_header)))
#define da_size(_da) ((_da) ? da_header(_da)->size : 0)
#define da_capacity(_da) ((_da) ? da_header(_da)->capacity : 0)
#define da_is_full(_da) ((_da) ? (da_size(_da) == da_capacity(_da)) : true)
#define da_push(_da, _it) \
do { \
    da_is_full(_da) ? (_da) = da_grow((_da), sizeof(*(_da))) : (void)0; \
    (_da)[da_header(_da)->size++] = (_it); \
} while(0);
#define da_reserve(_type, _size) da_reserve_(sizeof(_type), _size)
#define da_free(_da) da_free_((_da), sizeof(*(_da)))
void *da_reserve_(uint32_t stride, uint32_t count);
void *da_grow(void *a, uint32_t stride);
void da_free_(void *a, uint32_t stride);

#endif 