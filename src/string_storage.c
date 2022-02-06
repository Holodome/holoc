#include "string_storage.h"

#include <assert.h>
#include <string.h>

#include "allocator.h"
#include "llist.h"

static string_storage ss_;
string_storage *ss = &ss_;

string_storage *
get_string_storage(void) {
    return ss;
}

char *
ss_get_write_ptr(uint32_t write_size) {
    assert(!ss->ticket);
    ss->ticket = true;

    ss_chunk *chunk = ss->chunk_stack;
    if (!chunk || chunk->buf_used + write_size > STRING_STORAGE_CHUNK_SIZE) {
        chunk = aalloc(ss->a, sizeof(ss_chunk));
        LLIST_ADD(ss->chunk_stack, chunk);
    }

    return chunk->buf + chunk->buf_used;
}

void
ss_complete_write(uint32_t nbytes_written) {
    assert(ss->ticket);

    ss_chunk *chunk = ss->chunk_stack;
    assert(chunk->buf_used + nbytes_written < STRING_STORAGE_CHUNK_SIZE);
    chunk->buf_used += nbytes_written;

    ss->ticket = false;
}
