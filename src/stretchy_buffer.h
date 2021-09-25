// Author: Holodome
// Date: 13.09.2021 
// File: pkby/src/stream.h
// Version: 0
// 
// Defines std::vector - like structure for general purpose use.
// Although it is obvious that resizing arrays is slow, sometimes it is faster for development to use 
// one of the dynamic arrays and if perfomance becomes a bottleneck it can be switched for a better variant
#pragma once 
#include "general.h"

typedef struct {
    u32 size;
    u32 capacity;
} StretchyBufferHeader;

#define sb(_type) _type *
#define sb_header(_a) ((StretchyBufferHeader *)((u8 *)(_a) - sizeof(StretchyBufferHeader)))
#define sb_size(_a) (sb_header(_a)->size)
#define sb_full(_a) (sb_header(_a)->size == sb_header(_a)->capacity)
#define sb_push(_a, _it) \
    do { \
        sb_full(_a) ? (_a) = sb_grow((_a), sizeof(*(_a))) : (void)0; \
        (_a)[sb_header(_a)->size++] = (_it); \
    } while(0);
#define sb_new(_n, _type) sb_new_((_n), sizeof(_type))

void *sb_grow(void *old, uptr el_size);
void *sb_new_(uptr n, uptr el_size);