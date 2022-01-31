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

// Allocation from given freelist
#define FREELIST_ALLOC(_fl, _a)                                 \
    freelist_alloc_impl((void **)&(_fl),                        \
                        ((char *)&(_fl)->next - (char *)(_fl)), \
                        sizeof(*(_fl)), (_a))

// Free object to freelist
#define FREELIST_FREE(_fl, _node) LLIST_ADD(_fl, _node)

void *freelist_alloc_impl(void **flp, uintptr_t next_offset, uintptr_t size,
                          struct allocator *a);

#endif
