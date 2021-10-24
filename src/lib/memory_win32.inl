#include "general.h"
#include "memory.h"

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void *
mem_alloc(uptr size) {
    void *result = 0;
    result = malloc(size);
    assert(result);
    mem_zero(result, size);
    return result;
}

void
mem_free(void *ptr, uptr size) {
    (void)size;
    free(ptr);
}

Memory_Block *
os_alloc_block(uptr size, u32 flags) {
    uptr request_size = align_forward_pow2(size + sizeof(Memory_Block), MEM_DEFAULT_ALIGNMENT);
    void *result = VirtualAlloc(0, request_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(result);
    Memory_Block *block = result;
    block->size = size;
    block->base = (u8 *)(block + 1);
    return block;
}

void 
mem_free_block(Memory_Block *block) {
    int result = VirtualFree(block, block->size + sizeof(Memory_Block), MEM_RELEASE);
    assert(result == 0);
}
