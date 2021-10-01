#include <sys/mman.h>
#include <stdlib.h>

void *mem_alloc(uptr size) {
    void *result = 0;
    result = malloc(size);
    assert(result);
    mem_zero(result, size);
    return result;
}

void mem_free(void *ptr, uptr size) {
    free(ptr);
}

MemoryBlock *mem_alloc_block(uptr size) {
    uptr request_size = align_forward_pow2(size + sizeof(MemoryBlock), MEM_DEFAULT_ALIGNMENT);
    void *result = mmap(0, request_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    assert(result != MAP_FAILED);
    MemoryBlock *block = result;
    block->size = size;
    block->base = (u8 *)(block + 1);
    return block;
}

void mem_free_block(MemoryBlock *block) {
    int result = munmap(block, block->size + sizeof(MemoryBlock));
    assert(result == 0);
}