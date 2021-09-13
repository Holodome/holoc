#include "stream.h"
#include "memory.h"
#include "strings.h"
#define STREAM_NEEDS_FLUSH(_st) ((_st)->mode == STREAM_FILE && (_st)->bf_idx >= (_st)->threshold)

void init_console_streams(void);
extern InStream *stdin_stream;
extern OutStream *stdout_stream;
extern OutStream *stderr_stream;

// When workign with binary data and size of sizngle data block is bigger than
// threshold, data should be written directly
static void out_stream_write_direct(OutStream *st, const void *data, uptr data_sz) {
    if (st->mode == STREAM_FILE) {
        b32 written = write_file(st->out_file, st->out_file_idx, data, data_sz);
        assert(written);
        st->out_file_idx += data_sz;
    }
}

OutStream create_out_stream(void *bf, uptr bf_sz) {
    OutStream stream = {0};
    stream.mode = STREAM_BUFFER;
    stream.bf = bf;
    stream.bf_sz = bf_sz;
    return stream;
}

OutStream create_out_streamf(FileID file, uptr bf_sz, uptr threshold) {
    assert(threshold >= bf_sz);
    OutStream stream = {0};
    stream.mode = STREAM_FILE;
    stream.bf = mem_alloc(bf_sz);
    stream.bf_sz = bf_sz;
    stream.threshold = threshold;
    return stream;
}

void destroy_out_stream(OutStream *st) {
    if (st->mode == STREAM_FILE) {
        mem_free(st->bf);
    }
}

uptr out_streamf(OutStream *st, const char *format, ...) {
    va_list args;
    va_start(args, format);
    return out_streamv(st, format, args);
}

uptr out_streamv(OutStream *st, const char *format, va_list args) {
    assert(!STREAM_NEEDS_FLUSH(st));
    u32 bytes_written = vfmt((char *)st->bf + st->bf_idx, st->bf_sz - st->bf_idx, format, args);
    st->bf_idx += bytes_written;
    // this should never be hit due to nature of fmt, but assert in case fmt breaks
    assert(st->bf_idx < st->bf_sz);
    if (STREAM_NEEDS_FLUSH(st)) {
        out_stream_flush(st);
    }
    return bytes_written;   
}

void out_streamb(OutStream *st, const void *b, uptr c) {
    // decide writing policy 
    if (c <= st->bf_sz) {
        if (st->bf_idx + c >= st->bf_sz) {
            out_stream_flush(st);
        }
        mem_copy(st->bf, b, c);
        st->bf_idx = c;
        if (STREAM_NEEDS_FLUSH(st)) {
            out_stream_flush(st);
        }
    } else {
        out_stream_flush(st);
        out_stream_write_direct(st, b, c);
    }
}

void out_stream_flush(OutStream *st) {
    if (st->mode == STREAM_FILE) {
        assert(is_file_valid(st->out_file));
        out_stream_write_direct(st, st->bf, st->bf_idx);
        st->bf_idx = 0;
    }
}

