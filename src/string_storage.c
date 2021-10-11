#include "string_storage.h"

String_Storage *
create_string_storage(Memory_Arena *arena) {
    UNUSED(arena);
    return 0;
    // String_Storage *ss = arena_alloc_struct(arena, String_Storage);
    // ss->arena = arena;
    // ss->hash_table.num_buckets = STRING_STORAGE_HASH_SIZE;
    // ss->hash_table.keys = arena_alloc_array(arena, STRING_STORAGE_HASH_SIZE, u64);
    // ss->hash_table.values = arena_alloc_array(arena, STRING_STORAGE_HASH_SIZE, u64);
}

void 
string_storage_begin_write(String_Storage *storage) {
    UNUSED(storage);
}

void 
string_storage_write(String_Storage *storage, const void *bf, uptr bf_sz) {
    UNUSED(storage);
    UNUSED(bf);
    UNUSED(bf_sz);
}

String_ID 
string_storage_end_write(String_Storage *storage) {
        UNUSED(storage);

    return (String_ID){0};
}

// shorthand for writing calls for above 3 functions
String_ID 
string_storage_add(String_Storage *storage, const char *str) {
        UNUSED(storage);
        UNUSED(str);

    return (String_ID){0};
}

const char *
string_storge_get(String_Storage *storage, String_ID id) {
        UNUSED(storage);
    UNUSED(id);

    return 0;
}