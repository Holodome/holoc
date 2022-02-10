// Generic dynamic array (vector) implementation for C
#ifndef DARRAY_H
#define DARRAY_H

#include <stdbool.h>
#include <stdint.h>

#define DARRAY_DEFAULT_SIZE 10

// Structure allocated with dynamic array and placed before it in memory
// Holds meta information about array and can be accessed with da_header macro
typedef struct {
    uint32_t size;
    uint32_t capacity;
} darray_header;

// Returns header for given dynamic array
#define da_header(_da) ((darray_header *)((char *)(_da) - sizeof(darray_header)))
// Returns size of array
#define da_size(_da) ((_da) ? da_header(_da)->size : 0)
// Returns size of array in bytes
#define da_bytes(_da) ((_da) ? da_size(_da) * sizeof(*(_da)) : 0)
// Returns capacity of array
#define da_capacity(_da) ((_da) ? da_header(_da)->capacity : 0)
// Checks if dynamic array has no space left for new elements
#define da_is_full(_da) ((_da) ? (da_size(_da) == da_capacity(_da)) : true)
// Reserves space for n elements
#define da_reserve(_type, _size) da_reserve_(sizeof(_type), _size)
// Pushes 1 element
// _it must be the same type as array element (not pointer to it)
#define da_push(_da, _it)                           \
    do {                                            \
        if (da_is_full(_da)) {                      \
            (_da) = da_grow((_da), sizeof(*(_da))); \
        }                                           \
        (_da)[da_header(_da)->size++] = (_it);      \
    } while (0)
// Frees memory allocated by array
#define da_free(_da) ((_da) ? da_free_((_da), sizeof(*(_da))) : (void)0)
// Pops last element and returns it
#define da_pop(_da) ((_da)[--da_header(_da)->size])
// Returns pointer to last element
#define da_last(_da) ((_da) ? ((_da) + da_size(_da) - 1) : 0)

// Grows array geometrically or creates new
void *da_grow(void *da, uintptr_t stride);
// Internal use functions
void *da_reserve_(uintptr_t stride, uint32_t size);
void da_free_(void *da, uintptr_t stride);

#endif
