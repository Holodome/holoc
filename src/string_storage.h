/*
Author: Holodome
Date: 10.10.2021
File: src/string_storage.h
Version: 0

Provides memory for storing of strings for the program 
*/
#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H

#include "lib/general.h"
#include "lib/strings.h"

struct Compiler_Ctx;

#define STRING_STORAGE_BUFFER_SIZE KB(16)
#define STRING_STORAGE_HASH_SIZE   8192

typedef struct String_Storage_Buffer {
    u8  storage[STRING_STORAGE_BUFFER_SIZE];
    u32 used;
    
    struct String_Storage_Buffer *next;
} String_Storage_Buffer;

typedef struct String_Storage {
    struct Compiler_Ctx *ctx;
    
    u32 buffer_count;
    String_Storage_Buffer *first_buffer;
} String_Storage;

String_Storage *create_string_storage(struct Compiler_Ctx *ctx);
Str string_storage_add(String_Storage *storage, const char *str, u32 len);

#endif