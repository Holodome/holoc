#include "memory.h"

#include "strings.h"

#include <string.h> // memset, memcpy, memmove
#include <stdlib.h>

void *mem_alloc(uptr size) {
    void *result = 0;
    result = malloc(size);
    assert(result);
    memset(result, 0, size);
    return result;
}

void mem_free(void *ptr) {
    free(ptr);
}

void mem_copy(void *dst, const void *src, uptr size) {
    memcpy(dst, src, size);
}

void mem_move(void *dst, const void *src, uptr size) {
    memmove(dst, src, size);
}

void mem_zero(void *dst, uptr size) {
    memset(dst, 0, size);
}

MemoryBlock *mem_alloc_block(uptr size) {
    uptr total_size = size + sizeof(MemoryBlock);
    MemoryBlock *block = mem_alloc(total_size);
    block->size = size;
    block->base = (u8 *)block + sizeof(MemoryBlock);
    return block;
}

static uptr get_alignment_offset(MemoryArena *arena, uptr align) {
   assert(IS_POW2(align));
   uptr result_ptr = (uptr)arena->current_block->base + arena->current_block->used;
   uptr align_mask = align - 1;
   uptr offset = 0;
   if (result_ptr & align_mask) {
       offset = align - (result_ptr & align_mask);
   }
   return offset;
}

static uptr get_effective_size(MemoryArena *arena, uptr size) {
    return size + get_alignment_offset(arena, MEMORY_ARENA_DEFAULT_ALIGNMENT);
}

void *arena_alloc(MemoryArena *arena, uptr size_init) {
    void *result = 0;
    if (size_init) {
        uptr size = 0;
        if (arena->current_block) {
            size = get_effective_size(arena, size_init);
        }

        if (!arena->current_block || 
                (arena->current_block->used + size > arena->current_block->size)) {
            size = size_init;
            if (!arena->minimum_block_size) {
                arena->minimum_block_size = MEMORY_ARENA_DEFAULT_MINIMAL_BLOCK_SIZE;
            }

            uptr block_size = size;
            if (arena->minimum_block_size > block_size) {
                block_size = arena->minimum_block_size;
            }

            MemoryBlock *new_block = mem_alloc_block(block_size);
            LLIST_ADD(arena->current_block, new_block);
        }

        assert(arena->current_block->used + size < arena->current_block->size);
        uptr align_offset = get_alignment_offset(arena, MEMORY_ARENA_DEFAULT_ALIGNMENT);
        uptr block_offset = arena->current_block->used + align_offset;
        result = arena->current_block->base + block_offset;
        arena->current_block->used += size;
        assert(block_offset + size < arena->current_block->size);
        assert(size >= size_init);
    }
    return result;
}

void *arena_copy(MemoryArena *arena, const void *src, uptr size) {
    void *result = arena_alloc(arena, size);
    mem_copy(result, src, size);
    return result;
}

char *arena_alloc_str(MemoryArena *arena, const char *src) {
    uptr length = str_len(src);
    void *result = arena_alloc(arena, length + 1);
    mem_copy(result, src, length + 1);
    return result; 
}

void arena_free_last_block(MemoryArena *arena) {
    MemoryBlock *block = arena->current_block;
    LLIST_POP(arena->current_block);
    mem_free(block);
}
// Frees all blocks
void arena_clear(MemoryArena *arena) {
    while (arena->current_block) {
        // In case arena itself is stored in last block
        b32 is_last_block = (arena->current_block->next == 0);
        arena_free_last_block(arena);
        if (is_last_block) {
            break;
        }
    }
}

TemporaryMemory begin_temp_memory(MemoryArena *arena) {
    ++arena->temp_memory_count;
    TemporaryMemory result;
    result.arena = arena;
    result.block = arena->current_block;
    result.block_used = arena->current_block->used;
    return result;
}

void end_temp_memory(TemporaryMemory temp) {
    assert(temp.arena->temp_memory_count);
    --temp.arena->temp_memory_count;

    while (temp.arena->current_block != temp.block) {
        arena_free_last_block(temp.arena);
    }

    if (temp.arena->current_block) {
        assert(temp.arena->current_block->used >= temp.block_used);
        mem_zero(temp.arena->current_block->base + temp.block_used, temp.arena->current_block->used - temp.block_used);
        temp.arena->current_block->used = temp.block_used;
    }
}

