/*
Author: Holodome
Date: 21.11.2021
File: src/hashing.h
Version: 0
*/
#ifndef HASHING_H
#define HASHING_H

#include "general.h"

// Returns element of internal chaining hash table, given list of entries and
// hash value
// @NOTE(hl): This is actually the only low-level method function call needed to
// implement hash table
//  insertion to hash table can be done by writing hash value in resulting
//  pointer, and deletion can be done with setting hash value to zero
// or_zero being 1 means that hash table may use new slot for this hash
// @NOTE(hl): Entry count should be power of two
void *hash_table_oa_get_u32(void *entries, uint32_t entry_count, uintptr_t stride,
                            uintptr_t hash_offset, uint32_t hash, bool or_zero);
// Returns element of external chaining hash table, given list of pointers to
// entries Each entry may be a null or a valid pointer, in which case it also
// has next value, offset of which is specified using chain_offset Return value
// is always non-void pointer pointing to location of hash table element if
// value pointed to is zero, no element with given hash was found, but it could
// be created and written to returned location. Entry count should be power of
// two
#define hash_table_sc_get_u32(_entries, _entry_count, _struct, _chain_name, _hash_name, \
                              _hash)                                                    \
    (_struct **)hash_table_sc_get_u32_((void **)(_entries), _entry_count,               \
                                       offsetof(_struct, _chain_name),                  \
                                       offsetof(_struct, _hash_name), _hash)

void **hash_table_sc_get_u32_(void **entries, uint32_t entry_count, uintptr_t chain_offset,
                              uintptr_t hash_offset, uint32_t hash);

#define hash_string__(_key, _len, _seed) murmur3_32(_key, _len, _seed)
#define hash_string_(_string, _seed) hash_string__((_string).data, (_string).len, _seed)
#define hash_string(_string) hash_string_((_string), 0)
uint32_t murmur3_32(void *keyv, uint32_t len, uint32_t seed);

#endif
