// Author: Holodome
// Date: 25.08.2021 
// File: pkby/src/hashing.h
// Revisions: 0
// 
// Provides different APIs that are conncected with hashing.
#pragma once
#include "general.h"
#include "memory.h"

u32 hash_string(const char *str);
u32 crc32(u32 crc, const char *bf, uptr bf_sz);

// Type-agnostic implementatin of hash tables.
// Values are typycally indices of array that actually stores values in it.
// This hash structure design benefits in terms of cache coherency unlike straigtforward 
// way of doing this - having structure stored as value.
// 
// This of course needs some additional memory for storing indices array, but its 
// speed and cache-firendliness akes it more suitable for general-purpose hash table,
// and also avoids macros madness connected with inability to template in c
// Use cases that worry about memory can provide its own implementation of hash table
//
// Internal chaining hash table
// @NOTE It is almost always possible to have arbitrary sized hash tables and having size be power of two
// has hash lookup speed benefits - so it is a forced strategy in this implementation
// @NOTE 0 is not valid key - it is reserved to mark empty slots 
typedef struct Hash64 {
    u64 num_buckets;
    u64 *keys;
    u64 *values;
} Hash64;

Hash64 create_hash64(u64 n, MemoryArena *arena);
//  @NOTE creation and deletion is not specified - because different use cases may want to
// ahve different allocation strategies and we don't have API for that
b32 hash64_set(Hash64 *hash, u64 key, u64 value);
u64 hash64_get(Hash64 *hash, u64 key, u64 default_value);
// @NOTE internal chaining hash table is not suitable for deletion of items because of current implementation
// of setting. If we decide to add deletions, there should be two versions of set - set new and reset
// (because if bucket is chained, and item before it but after its hash idx is deleted program decides to
// create new slot for this item, despite having it already stored)
// This is complicated to test at this development stage so it is better to just not have deletions