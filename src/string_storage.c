#include "string_storage.h"
#include "lib/strings.h"

StringStorage * 
create_string_storage(u32 hash_size, MemoryArena *arena) {
    StringStorage *storage = arena_alloc_struct(arena, StringStorage);
    storage->arena = arena;
    storage->hash = create_hash64(hash_size, storage->arena);    
    return storage;
}

StringID string_storage_add(StringStorage *storage, const char *str) {
    StringID id = {0};
    u64 hash = hash_string(str);
    assert(hash);
    
    u64 default_value = (u64)-1;
    u64 string_buffer_idx = hash64_get(&storage->hash, hash, default_value);
    if (string_buffer_idx != default_value) {
        id.value = string_buffer_idx;        
    } else {
        uptr len = str_len(str) + 1;
        if (!storage->first_buffer || (storage->first_buffer->used + len > STRING_STORAGE_BUFFER_SIZE )) {
            StringStorageBuffer *new_buf = arena_alloc_struct(storage->arena, StringStorageBuffer);
            LLIST_ADD(storage->first_buffer, new_buf);
            ++storage->buffer_count;
        }

        StringStorageBuffer *buf = storage->first_buffer;
        assert(buf && buf->used + len <= STRING_STORAGE_BUFFER_SIZE);
        
        string_buffer_idx = ((u64)storage->buffer_count << 32) | ((u64)buf->used);
        
        mem_copy(buf->storage + buf->used, str, len);
        buf->used += len;
        id.value = string_buffer_idx;
    }

    return id;
} 

const char *string_storage_get(StringStorage *storage, StringID id) {
    const char *result = 0;
    u64 default_value = (u64)-1;
    u64 buffer_string_idx = hash64_get(&storage->hash, id.value, default_value);
    if (buffer_string_idx != default_value) {
        u32 buffer_idx = buffer_string_idx >> 32;
        u32 string_idx = buffer_string_idx & 0xFFFFFFFF;
        StringStorageBuffer *buf = storage->first_buffer;
        u32 cursor = storage->buffer_count - 1;
        while (cursor != buffer_idx) {
            --cursor;
            buf = buf->next;
        }

        result = (char *)buf->storage + string_idx;
    }
    return result;
}
