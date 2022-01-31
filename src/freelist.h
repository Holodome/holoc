// This file defines macros used when working with freelist-based allocator.
// The core concept is that a linked list of objects is stored somewhere, which
// is called freelist. When object of that type needs to be allocated, it can
// either be popped from freelist and thus reuse old memory or allocate new
// using allocator.
//
// This approach (theoretically) is faster than using dynamic allocator (like
// malloc). Thus, it is important to keep code that uses this functionality
// readable and easy to find. Because of that it has been decided to use macros.
//
// If one day we want to check actual perfomance, these macros can be easilly
// replaced.
#ifndef FREELIST_H
#define FREELIST_H

#include "types.h"

struct allocator;

#if 1
// Allocation from given freelist
// HL 31.01.22 This code derefernces _fl that may be null pointer. This is
// unclear whether this is considred a defined behaviour or not, but compilers
// generate code that does not actually do dereferencing. When accessing member,
// it adds offset of that member to the location of the struct head.
// https://godbolt.org/z/hKvsx38f9
#define FREELIST_ALLOC(_flp, _a)                                        \
    freelist_alloc_impl((void **)(_flp),                                \
                        ((char *)&(*(_flp))->next - (char *)(*(_flp))), \
                        sizeof(**(_flp)), (_a))

// Free object to freelist
#define FREELIST_FREE(_fl, _node) LLIST_ADD(_fl, _node)
#else 
#define FREELIST_ALLOC(_flp, _a) aalloc(_a, sizeof(**(_flp)))
#define FREELIST_FREE(_fl, _node) (void)0
#endif 

void *freelist_alloc_impl(void **flp, uintptr_t next_offset, uintptr_t size,
                          struct allocator *a);

#endif
