#include <stdlib.h>

void mem_free(void *ptr, uptr size) {
    free(ptr);
}

void *mem_alloc(uptr size) {
    void *result = 0;
    result = malloc(size);
    assert(result);
    mem_zero(result, size);
    return result;
}

MemoryBlock *mem_alloc_block(uptr size) {
    uptr total_size = size + sizeof(MemoryBlock);
    MemoryBlock *block = mem_alloc(total_size);
    block->size = size;
    block->base = (u8 *)block + sizeof(MemoryBlock);
    return block;
}