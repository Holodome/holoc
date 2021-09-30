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
#include "lib/general.h"
#include "lib/hashing.h"

#define STRING_STORAGE_BUFFER_SIZE KB(16)
#define STRING_STORAGE_HASH_SIZE 8192
#define STRING_STORAGE_NUGE (1llu << 63)

// @TODO(hl): Do we want to store some metadata with the string (like length,hash)
typedef struct {
    u64 value;
} StringID;

typedef struct StringStorageBuffer {
    u8 storage[STRING_STORAGE_BUFFER_SIZE];
    u16 used;
    
    struct StringStorageBuffer *next;
} StringStorageBuffer;

typedef struct {
    u32 length;
} StringMetaData;

typedef struct {
    MemoryArena *arena;

    Hash64 hash;
    StringStorageBuffer *first_buffer;
    StringStorageBuffer *first_free_buffer;
    u32 buffer_count; // @NOTE(hl): Used to get buffer index
    
    b32 is_inside_write;
    u64 current_write_start;
    u32 current_write_len;
    u32 current_write_crc;
} StringStorage;

StringStorage *create_string_storage(u32 hash_size, MemoryArena *arena);
void string_storage_begin_write(StringStorage *storage);
void string_storage_write(StringStorage *storage, const void *bf, u32 bf_sz);
StringID string_storage_end_write(StringStorage *ss);
u32 string_storage_get(StringStorage *ss, StringID id, void *bf, uptr bf_sz);
StringID string_storage_add(StringStorage *storage, const char *str);