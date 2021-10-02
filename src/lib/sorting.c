#include "sorting.h"

u64 f32_to_sort_key(f32 value) {
    u32 result = *(u32 *)&value;
    if (result & 0x80000000) {
        result = ~result;
    } else {
        result |= 0x80000000;
    }
    return (u64)result;
}

void radix_sort(SortEntry *entries, SortEntry *sort_temp, uptr n) {
    SortEntry *src = entries;
    SortEntry *dst = sort_temp;
    for (u32 byte_idx = 0; byte_idx < 32; byte_idx += 8) {
        u32 sort_key_offsets[256] = {0};
        for (u32 i = 0; i < n; ++i) {
            u32 radix_value = src[i].key;
            u32 radix_piece = (radix_value >> byte_idx) & 0xFF;
            ++sort_key_offsets[radix_piece];
        }
        
        u32 total = 0;
        for (u32 sort_key_idx = 0; sort_key_idx < ARRAY_SIZE(sort_key_offsets); ++sort_key_idx) {
            u32 count = sort_key_offsets[sort_key_idx];
            sort_key_offsets[sort_key_idx] = total;
            total += count;
        }
        
        for (u32 i = 0; i < n; ++i) {
            u32 radix_value = src[i].key;
            u32 radix_piece = (radix_value >> byte_idx) & 0xFF;
            dst[sort_key_offsets[radix_piece]++] = src[i];
        }
        
        SortEntry *temp = dst;
        dst = src;
        src = temp;
    }
}