/*
Author: Holodome
Date: 10.10.2021
File: src/string_storage.h
Version: 0

Acceleration structure for string storage.

@NOTE(hl):
C specification 5.2.4.1  Translation limits
- 4095 characters in a string literal
However, neither C nor C++ specify maximum identifier length.
Ley that just be the same of string's
*/
#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H
#include "lib/general.h"
#include "lib/hashing.h"

struct Memory_Arena;

// @NOTE(hl): Any operations with strings are hugely ineffective, considering that most of the times
// they are just need for comparison of some sort.
// Becuase of that, strings are stored in special storage structure dedicated for them, 
// and can be accessed by id;
typedef struct {
    u64 opaque;
} String_ID;

#define STRING_STORAGE_BUFFER_SIZE KB(16)
// Basically maximum number of strings allowed.
#define STRING_STORAGE_HASH_SIZE   8192

enum {
    STRING_ASCII = 0x0,
    STRING_UTF32 = 0x1,
};

typedef struct {
    u32 len;
    u32 crc;
    u8  mode;
} __attribute__((packed, aligned(1))) String_Storage_String_Metadata;

typedef struct String_Storage_Buffer {
    u8 storage[STRING_STORAGE_BUFFER_SIZE];
    u32 used;
    u32 index;
    struct String_Storage_Buffer *next;
} String_Storage_Buffer;

typedef struct String_Storage {
    struct Memory_Arena *arena;
    
    // Bufferizes storage of strings.
    // Each buffer contains packed binary data about strings.
    // String_ID is used to store location in buffer.
    // Linked lists for buffers. The last created buffer is generally going to be found on top (besides some corner cases that
    // are _never_ going to happen). Because of that, access time for writing is extremely low
    String_Storage_Buffer *buffer;
    u32 buffer_count;
    String_Storage_Buffer *first_free_buffer;
    
    // Hash table is used to store information whether string with current hash
    // is already in the table, or should be written as new one
    Hash_Table64 hash_table;
    
    bool is_inside_write;
    String_ID current_write_id;
    u32 current_write_len;
    u32 current_write_crc;
} String_Storage;

String_Storage *create_string_storage(struct Memory_Arena *arena);
void string_storage_begin_write(String_Storage *storage);
void string_storage_write(String_Storage *storage, const void *bf, u32 bf_sz);
String_ID string_storage_end_write(String_Storage *storage);
// shorthand for writing calls for above 3 functions
String_ID string_storage_add(String_Storage *storage, const char *str);
const char *string_storage_get(String_Storage *storage, String_ID id);

#endif