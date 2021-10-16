// Author: Holodome
// Date: 25.08.2021 
// File: pkby/src/lib/hashing.h
// Version: 0
// 
// Provides different APIs that are conncected with hashing.
#pragma once
#include "lib/general.h"

u32 djb2(const void *bf, uptr bf_sz);
u32 crc32(u32 crc, const void *bf, uptr bf_sz);
u64 fnv64(const void *bf, uptr bf_sz);

// Hash table with 64 bit hashes.
typedef struct Hash_Table64 {
    u32 num_buckets;
    u64 *keys;
    u64 *values;
} Hash_Table64;

typedef struct {
    bool is_valid;
    u64 value;
    // u32 idx;
} Hash_Table64_Get_Result;

Hash_Table64_Get_Result hash_table64_get(Hash_Table64 *table, u64 key);
bool hash_table64_set(Hash_Table64 *table, u64 key, u64 value);
bool hash_table64_delete(Hash_Table64 *table, u64 key);