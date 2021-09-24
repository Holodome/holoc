#include "stretchy_buffer.h"
#include "memory.h"

void *sb_grow(void *old, uptr el_size) {
    u32 old_size = sb_header(old)->capacity;
    u32 new_size = old_size * 2;
    sb_header(old)->capacity = new_size;
    return mem_realloc(old, old_size * el_size, new_size * el_size);
}

void *sb_new_(void *n, uptr el_size) {
    
}