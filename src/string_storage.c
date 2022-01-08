#include "string_storage.h"

#include "bump_allocator.h"
#include "my_assert.h"

#include <stddef.h>
#include <string.h>

static string_storage_buffer *
get_effective_buffer(string_storage *ss, uint32_t size) {
   assert(size <= STRING_STORAGE_BUFFER_SIZE);
   string_storage_buffer *buffer = ss->first_buffer;
   if (!buffer || (buffer->used + size > STRING_STORAGE_BUFFER_SIZE)) {
        string_storage_buffer *new_buffer = bump_alloc(ss->allocator, sizeof(string_storage_buffer));
        new_buffer->next = buffer;
        ss->first_buffer = new_buffer;
   }

   return buffer;
}

string_storage *
init_string_storage(struct bump_allocator *allocator) {
    string_storage *ss = bump_alloc(allocator, sizeof(string_storage));
    ss->allocator = allocator;
    return ss;
}

string 
string_storage_add(string_storage *ss, char *str, uint32_t len) {
    uint32_t size = len + 1 + sizeof(uint32_t);
    string_storage_buffer *buffer = get_effective_buffer(ss, size);
    string_storage_string *storage = (string_storage_string *)buffer->data + buffer->used;
    storage->size = len;
    memcpy(storage->data, str, len);
    storage->data[len] = 0;
    buffer->used += size;

    string result;
    result.len = len;
    result.data = storage->data;
    return result;
}


