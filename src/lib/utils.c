#include "utils.h"

inline u32 align_to_next_pow2(u32 v) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

inline u64 align_forward(u64 value, u64 align) {
    u64 result = (value + (align - 1)) / align * align;
    return result;
}

inline u64 align_forward_pow2(u64 value, u64 align) {
    assert(IS_POW2(align));
    u64 align_mask = align - 1;
    u64 result = (value + align_mask) & ~align_mask;
    return result;
}

inline u64 align_backward_pow2(u64 value, u64 align) {
    assert(IS_POW2(align));
    u64 align_mask = align - 1;
    u64 result = value & ~align_mask;
    return result;
}