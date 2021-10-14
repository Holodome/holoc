#include "lib/memory.h"

#include "lib/strings.h"
#include "lib/lists.h"

#include <string.h> // memset, memcpy, memmove

void *
mem_realloc(void *ptr, uptr old_size, uptr size) {
    void *new_ptr = mem_alloc(size);
    u32 size_to_copy = old_size;
    if (size < size_to_copy) {
        size_to_copy = size;
    }
    mem_copy(new_ptr, ptr, size_to_copy);
    mem_free(ptr, old_size);
    return new_ptr;
}

char *
mem_alloc_str(const char *str) {
    uptr len = str_len(str) + 1;
    char *result = mem_alloc(len);
    mem_copy(result, str, len);
    return result;    
}

void 
mem_copy(void *dst, const void *src, uptr size) {
    memcpy(dst, src, size);
}

void 
mem_move(void *dst, const void *src, uptr size) {
    memmove(dst, src, size);
}

void 
mem_zero(void *dst, uptr size) {
    memset(dst, 0, size);
}

bool 
mem_eq(const void *a, const void *b, uptr n) {
    return memcmp(a, b, n) == 0;
}

static uptr 
get_alignment_offset(Memory_Arena *arena, uptr align) {
   assert(IS_POW2(align));
   uptr result_ptr = (uptr)arena->current_block->base + arena->current_block->used;
   uptr align_mask = align - 1;
   uptr offset = 0;
   if (result_ptr & align_mask) {
       offset = align - (result_ptr & align_mask);
   }
   return offset;
}

static uptr 
get_effective_size(Memory_Arena *arena, uptr size) {
    return size + get_alignment_offset(arena, MEM_DEFAULT_ALIGNMENT);
}

void *
arena_alloc(Memory_Arena *arena, uptr size_init) {
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
                arena->minimum_block_size = MEM_DEFUALT_ARENA_BLOCK_SIZE;
            }

            uptr block_size = size;
            if (arena->minimum_block_size > block_size) {
                block_size = arena->minimum_block_size;
            }

            Memory_Block *new_block = mem_alloc_block(block_size);
            STACK_ADD(arena->current_block, new_block);
        }

        assert(arena->current_block->used + size <= arena->current_block->size);
        uptr align_offset = get_alignment_offset(arena, MEM_DEFAULT_ALIGNMENT);
        uptr block_offset = arena->current_block->used + align_offset;
        result = arena->current_block->base + block_offset;
        arena->current_block->used += size;
        assert(block_offset + size <= arena->current_block->size);
        assert(size >= size_init);
    }
    return result;
}

void *
arena_copy(Memory_Arena *arena, const void *src, uptr size) {
    void *result = arena_alloc(arena, size);
    mem_copy(result, src, size);
    return result;
}

char *
arena_alloc_str(Memory_Arena *arena, const char *src) {
    uptr length = str_len(src);
    void *result = arena_alloc(arena, length + 1);
    mem_copy(result, src, length + 1);
    return result; 
}

void 
arena_free_last_block(Memory_Arena *arena) {
    Memory_Block *block = arena->current_block;
    STACK_POP(arena->current_block);
    mem_free_block(block);
}
// Frees all blocks
void 
arena_clear(Memory_Arena *arena) {
    while (arena->current_block) {
        // In case arena itself is stored in last block
        bool is_last_block = (arena->current_block->next == 0);
        arena_free_last_block(arena);
        if (is_last_block) {
            break;
        }
    }
}

void *
arena_bootstrap_(uptr size, uptr arena_offset) {
    Memory_Arena bootstrap = {0};
    Memory_Arena *mem_ptr = arena_alloc(&bootstrap, size + sizeof(Memory_Arena));
    void *struct_ptr = mem_ptr + 1;
    *(Memory_Arena **)((u8 *)struct_ptr + arena_offset) = mem_ptr;
    return struct_ptr;
}

TemporaryMemory 
begin_temp_memory(Memory_Arena *arena) {
    ++arena->temp_memory_count;
    TemporaryMemory result;
    result.arena = arena;
    result.block = arena->current_block;
    result.block_used = arena->current_block->used;
    return result;
}

void 
end_temp_memory(TemporaryMemory temp) {
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

#if MEM_USE_STDLIB
#include "lib/memory_libc.inl"
#else 
#if OS_WINDOWS
#include "memory_win32.inl"
#elif OS_POSIX 
#include "memory_posix.inl"
#else 
#error !
#endif 
#endif 