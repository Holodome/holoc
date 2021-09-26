// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/ast.h
// Version: 0.1
// 
// Defines structure for accelerated string manipulation throughtout the compiler
// All strings are stored in one place, and there are no duplcates
// Strings can be referenced with single number and none string comparison is done afther tokenizing stage
#pragma once 
#include "lib/general.h"
#include "lib/hashing.h"

#define STRING_STORAGE_BUFFER_SIZE KB(4)
#define STRING_STORAGE_HASH_SIZE 8192

typedef struct {
    u64 value;
} StringID;

typedef struct StringStorageBuffer {
    u8 storage[STRING_STORAGE_BUFFER_SIZE];
    u16 used;
    
    struct StringStorageBuffer *next;
} StringStorageBuffer;

typedef struct {
    MemoryArena *arena;

    Hash64 hash;
    StringStorageBuffer *first_buffer;
    u32 buffer_count;
} StringStorage;

void init_string_storage(StringStorage *storage, u32 hash_size, MemoryArena *arena);
StringID string_storage_add(StringStorage *storage, const char *str);
const char *string_storage_get(StringStorage *storage, StringID id);
