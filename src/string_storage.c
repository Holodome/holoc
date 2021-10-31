#include "string_storage.h"

#include "lib/strings.h"
#include "lib/lists.h"
#include "lib/memory.h"

#include "compiler_ctx.h"

static String_Storage_Buffer *
get_buffer_for_writing(String_Storage *ss, u32 size) {
    String_Storage_Buffer *buffer = ss->first_buffer;
    if (!buffer || buffer->used + size > STRING_STORAGE_BUFFER_SIZE) {
        buffer = arena_alloc_struct(ss->ctx->arena, String_Storage_Buffer);
        STACK_ADD(ss->first_buffer, buffer);
        ++ss->buffer_count;
    } 
    return buffer;
}

String_Storage *
create_string_storage(struct Compiler_Ctx *ctx) {
    String_Storage *storage = arena_alloc_struct(ctx->arena, String_Storage);
    storage->ctx = ctx;
    return storage;
}

Str 
string_storage_add(String_Storage *ss, const char *str, u32 len) {
    Str result = {0};
    
    String_Storage_Buffer *buffer = get_buffer_for_writing(ss, len + 1);
    result.data = (const char *)buffer->storage + buffer->used;
    result.len = len;
    
    mem_copy(buffer->storage + buffer->used, str, len);
    buffer->storage[buffer->used + len] = 0;
    buffer->used += len;
    
    return result;
}
