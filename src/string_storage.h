/* 
Author: Holodome
Date: 25.09.2021 
File: pkby/src/ast.h
Version: 1
 
Defines structure for accelerated string manipulation throughtout the compiler
All strings are stored in one place, and there are no duplcates
Strings can be referenced with single number and none string comparison is done afther tokenizing stage
 
String storage should have following qualities:
1) Fast addition of arbitrary-sized strings
2) Storing strings location and access by id
3) Fast acess

Naive implementation would be to store strings in array, where each string has individual size
Because of that, having each string be allocated will lead to huge memory fragmentation due 
to alignment of memory allocators.
Because of that, it was decided to have strings be stored in huge buffers.
This, however, raises some complications in storing strings with sizes bigger than buffer size.
It was decided to be handled not ot break the buffer strategy - big strings are split into several
buffers, allowing to have no holes in buffer storage.
This makes accessing these kind of strings slow, but because their count is low, we don't care.

Because of this implementation, two values are needed to access string. The buffer index, and index of buffer in string.
Access time for buffer is O(n), but if buffer sizes big enough are chosed, this time is kept low.
*/
#pragma once 
#include "common.h"
#include "lib/hashing.h"

#define STRING_STORAGE_BUFFER_SIZE KB(16)
#define STRING_STORAGE_HASH_SIZE 8192
#define STRING_STORAGE_NUGE (1llu << 63)

typedef struct String_Storage_Buffer {
    u8 storage[STRING_STORAGE_BUFFER_SIZE];
    u16 used;
    
    struct String_Storage_Buffer *next;
} String_Storage_Buffer;

typedef struct String_Storage {
    Memory_Arena *arena;

    Hash64 hash;
    String_Storage_Buffer *first_buffer;
    String_Storage_Buffer *first_free_buffer;
    u32 buffer_count; // @NOTE(hl): Used to get buffer index
    
    bool is_inside_write;
    u64 current_write_start;
    u32 current_write_len;
    u32 current_write_crc;
} String_Storage;

String_Storage *create_string_storage(Memory_Arena *arena);
void string_storage_begin_write(String_Storage *storage);
void string_storage_write(String_Storage *storage, const void *bf, u32 bf_sz);
String_ID string_storage_end_write(String_Storage *ss);
u32 string_storage_get(String_Storage *ss, String_ID id, void *bf, uptr bf_sz);
String_ID string_storage_add(String_Storage *storage, const char *str);