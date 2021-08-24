#pragma once
#include "general.h"

// Implementation of radix sort.
// Has limitation of only operating on numerical values - so all sort entries have to be represented as numbers.
// Using radix sort can become too verbose in some string-heavy places, so another sorting algorithm may be introduced

typedef struct SortEntry {
    u64 key;   // value that array needs to be sorted around. Floating-point needs special handling, see functions below
    u64 value; // any value that user can use to use sort data
} SortEntry;
// Constructing of keys for floating-point values
u64 f32_to_sort_key(f32 value);
u64 f64_to_sort_key(f64 value);

// O(n)
void radix_sort(SortEntry *entries, SortEntry *temp, uptr n);