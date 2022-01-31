#include "freelist.h"

#include "allocator.h"
#include <string.h>


void *
freelist_alloc_impl(void **flp, uintptr_t next_offset, uintptr_t size,
                    allocator *a) {
    void *result = *flp;
    if (!result) {
        result = aalloc(a, size);
    } else {
        *flp = *(void **)((char *)result + next_offset);
        memset(result, 0, size);
    }
    return result;
}
