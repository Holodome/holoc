#include "bump_allocator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"

#define ALIGN 16

static uint64_t
align_forward_pow2(uint64_t value, uint64_t align) {
    uint64_t align_mask = align - 1;
    assert(!(align_mask & align));
    uint64_t align_value = value & align_mask;
    uint64_t result      = value;
    if (align_mask) {
        result += align - align_value;
    }
    return result;
}

static bump_allocator_block *
alloc_block(uint64_t size, uint64_t align) {
    uint64_t size_to_allocate =
        align_forward_pow2(size + sizeof(bump_allocator_block), align);
    void *memory = aligned_alloc(align, size_to_allocate);
    memset(memory, 0, size_to_allocate);
    bump_allocator_block *block = memory;

    block->data = (uint8_t *)align_forward_pow2(
        (uint64_t)(uint8_t *)memory + sizeof(bump_allocator_block), align);
    block->size = size;
    return block;
}

static void
bump_free_last_block(bump_allocator *allocator) {
    bump_allocator_block *block = allocator->block;
    allocator->block            = block->next;
    free(block);
}

void *
bump_bootstrap__(uintptr_t offset, uint64_t size) {
    bump_allocator allocator = {0};

    void *memory = bump_alloc(&allocator, size + sizeof(allocator));
    memcpy(memory, &allocator, sizeof(allocator));
    void *result                = (uint8_t *)memory + sizeof(allocator);
    *(void **)(result + offset) = memory;
    return result;
}

void *
bump_alloc(bump_allocator *allocator, uint64_t size) {
    void *result = NULL;
    if (!size) {
        goto end;
    }
    bump_allocator_block *block = allocator->block;
    // NOTE: If align = 0, size_aligned is 0
    // Because after frist block allocation allocator->aling is not zero, zero
    // size_aligned implies no block. So check !size_aligned is equal to !block
    // However, one could set align but still have no allocations.
    // So block must be additionally checked.
    uint64_t size_aligned = align_forward_pow2(size, ALIGN);
    if (!block || block->used + size_aligned > block->size) {
        // This is considered a rarely executed case, so this checks don't have
        // big perfomance impact
        if (!allocator->minimal_block_size) {
            allocator->minimal_block_size = BUMP_DEFAULT_MINIMAL_BLOCK_SIZE;
        }
        uint64_t block_size = size_aligned;
        if (allocator->minimal_block_size > block_size) {
            block_size = allocator->minimal_block_size;
        }

        block            = alloc_block(block_size, ALIGN);
        block->next      = allocator->block;
        allocator->block = block;
    }
    uint64_t align_mask = ALIGN - 1;
    // Make sure block start is aligned
    assert(!((uint64_t)block->data & align_mask));
    uint64_t start_offset = (uint64_t)block->data + block->used;
    // This should be aligned due to later operations, but add check just in
    // case
    assert(!(start_offset & align_mask));
    result = block->data + block->used;
    block->used += size_aligned;
    // Make sure resulting used value is properly aligned
    assert(!(block->used & align_mask));
end:
    return result;
}

void
bump_clear(bump_allocator *allocator) {
    while (allocator->block) {
        bool is_last = allocator->block->next == 0;
        bump_free_last_block(allocator);
        if (is_last) {
            break;
        }
    }
}

temp_memory
bump_temp(bump_allocator *allocator) {
    temp_memory temp;
    temp.allocator = allocator;
    temp.block     = allocator->block;
    if (temp.block) {
        temp.block_used = allocator->block->used;
    }
    ++allocator->temp_memory_count;
    return temp;
}

void
bump_temp_end(temp_memory temp) {
    assert(temp.allocator->temp_memory_count);
    --temp.allocator->temp_memory_count;

    while (temp.allocator->block != temp.block) {
        bump_free_last_block(temp.allocator);
    }

    if (temp.allocator->block) {
        assert(temp.allocator->block->used >= temp.block_used);
        memset(temp.allocator->block->data + temp.block_used, 0,
               temp.allocator->block->used - temp.block_used);
        temp.allocator->block->used = temp.block_used;
    }
}

static ALLOCATOR_REALLOC(bump_realloc) {
    bump_allocator *a = internal;

    void *new_ptr = 0;
    if (new_size) {
        new_ptr = bump_alloc(a, new_size);
    }
    return new_ptr;
}

struct allocator
bump_get_allocator(bump_allocator *a) {
    allocator all = {a, bump_realloc};
    return all;
}
