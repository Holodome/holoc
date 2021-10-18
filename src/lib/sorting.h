// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/lib/sorting.h
// Version: 0
//
// Provides different soring algorithms
#ifndef SORTING_H
#define SORTING_H
#include "lib/general.h"

// Implementation of radix sort.
// Has limitation of only operating on numerical values - so all sort entries have to be represented as numbers.
// Using radix sort can become too verbose in some string-heavy places, so another sorting algorithm may be introduced

typedef struct Sort_Entry {
    u32 key;   // value that array needs to be sorted around. Floating-point needs special handling, see functions below
    u32 value; // any value that user can use to use sort data
} Sort_Entry;
// Construct soring key from floating-pointvalue
u32 f64_to_sort_key(f64 value);

// O(n)
// Sorted result is stored in entries
void radix_sort(Sort_Entry *entries, Sort_Entry *temp, uptr n);

#endif