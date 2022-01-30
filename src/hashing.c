#include "hashing.h"

#include <assert.h>
#include <stddef.h>

void *
hash_table_oa_get_u32(void *entries, uint32_t entry_count, uintptr_t stride,
                      uintptr_t hash_offset, uint32_t hash, bool or_zero) {
    void *result       = NULL;
    uint32_t hash_mask = entry_count - 1;
    assert(!(hash_mask & entry_count));
    for (uint32_t offset = 0; offset < entry_count; ++offset) {
        uint32_t hash_slot_idx = (hash + offset) & hash_mask;
        void *hash_bucket      = ((uint8_t *)entries + hash_slot_idx * stride);
        void *hash_bucket_hash_ptr = (uint8_t *)hash_bucket + hash_offset;
        uint32_t hash_bucket_hash  = *(uint32_t *)hash_bucket_hash_ptr;
        if (hash_bucket_hash == hash || (or_zero && !hash_bucket_hash)) {
            result = hash_bucket;
            break;
        }
    }

    return result;
}

void **
hash_table_sc_get_u32_(void **entries, uint32_t entry_count,
                       uintptr_t chain_offset, uintptr_t hash_offset,
                       uint32_t hash) {
    uint32_t hash_mask = entry_count - 1;
    assert(!(hash_mask & entry_count));
    uint32_t hash_slot_idx = hash & hash_mask;
    void **entry           = entries + hash_slot_idx;
    while (*entry) {
        void *entry_hash_ptr = (char *)*entry + hash_offset;
        uint32_t entry_hash  = *(uint32_t *)entry_hash_ptr;
        if (entry_hash == hash) {
            break;
        } else {
            entry = (void **)((char *)*entry + chain_offset);
        }
    }

    return entry;
}

static uint32_t
murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t
murmur3_32(void *keyv, uint32_t len, uint32_t seed) {
    uint8_t *key = (uint8_t *)keyv;
    uint32_t h   = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (uint32_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        k = *(uint32_t *)key;
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (uint32_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}
