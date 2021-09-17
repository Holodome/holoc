#include "stream.h"
#include "memory.h"
#include "strings.h"
#define STREAM_NEEDS_FLUSH(_st) ((_st)->mode == STREAM_FILE && (_st)->bf_idx >= (_st)->threshold)

// When workign with binary data and size of sizngle data block is bigger than
// threshold, data should be written directly
static void out_stream_write_direct(OutStream *st, const void *data, uptr data_sz) {
    if (st->mode == STREAM_FILE) {
        uptr written = write_file(st->out_file, st->out_file_idx, data, data_sz);
        st->out_file_idx += written;
    }
}

OutStream create_out_stream(void *bf, uptr bf_sz) {
    OutStream stream = {0};
    stream.mode = STREAM_BUFFER;
    stream.bf = bf;
    stream.bf_sz = bf_sz;
    return stream;
}

OutStream create_out_streamf(FileID file, uptr bf_sz, uptr threshold, b32 is_std) {
    assert(threshold < bf_sz);
    OutStream stream = {0};
    stream.out_file = file;
    if (is_std) {
        stream.mode = STREAM_ST;
    } else {
        stream.mode = STREAM_FILE;
    }
    stream.bf = mem_alloc(bf_sz);
    stream.bf_sz = bf_sz;
    stream.threshold = threshold;
    return stream;
}

void destroy_out_stream(OutStream *st) {
    out_stream_flush(st);
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
    if (st->mode == STREAM_BUFFER) {
        // nop  
    } else if (st->mode == STREAM_FILE) {
        out_stream_write_direct(st, st->bf, st->bf_idx);
        st->bf_idx = 0;
    } else if (st->mode == STREAM_ST) {
        write_file(st->out_file, 0xFFFFFFFF, st->bf, st->bf_idx);
        st->bf_idx = 0;
    }
}

InStream create_in_stream(void *bf, uptr bf_sz) {
    InStream st = {0};
    st.mode = STREAM_BUFFER;
    st.bf = bf;
    st.bf_sz = bf_sz;
    return st;
}

InStream create_in_streamf(FileID file, uptr bf_sz, uptr threshold, b32 is_std) {
    assert(bf_sz > threshold);
    InStream st = {0};
    st.file_size = get_file_size(file);
    if (is_std) {
        st.mode = STREAM_ST;
    } else {
        st.mode = STREAM_FILE;
    }
    st.bf = mem_alloc(bf_sz);
    st.bf_sz = bf_sz;
    st.threshold = threshold;
    return st;
}

void destroy_in_stream(InStream *st) {
    if (st->mode == STREAM_FILE) {
        mem_free(st->bf);
    }
}

uptr in_stream_peek(InStream *st, void *out, uptr n) {
    uptr result = 0;
    if (st->bf_idx + n < st->bf_used) {
        mem_copy(out, st->bf + st->bf_idx, n);
        result = n;
    } else {
        // @TODO
        NOT_IMPLEMENTED;
    }
    return result;
}

uptr in_stream_advance_n(InStream *st, uptr n) {
    uptr result = 0;
    if (st->bf_idx + n < st->bf_used) {
        st->bf_idx += n;
        result += n;
        
        if (st->bf_idx > st->threshold) {
            in_stream_flush(st);
        }
    } else {
        // @TODO 
        NOT_IMPLEMENTED;
    }
    return result;
}

void in_stream_flush(InStream *st) {
    if (st->mode == STREAM_BUFFER) {
        // nop
    } else if (st->mode == STREAM_FILE) {
        // Move chunk of file that is not processed to buffer start
        assert(st->bf_idx < st->bf_sz);
        mem_move(st->bf, st->bf + st->bf_idx, st->bf_used - st->bf_idx);
        st->bf_used = st->bf_idx;
        st->bf_idx = 0;
        // Read new data
        uptr buffer_size_aviable = st->bf_sz - st->bf_used;
        uptr read_data_size = st->file_size - st->file_idx;
        if (read_data_size > buffer_size_aviable) {
            read_data_size = buffer_size_aviable;
        }
        uptr bytes_read = read_file(st->file, st->file_idx, st->bf + st->bf_used, read_data_size);
        st->bf_used += bytes_read;
        st->file_idx += bytes_read;
    } else if (st->mode == STREAM_ST) {
        // Move chunk of file that is not processed to buffer start
        assert(st->bf_idx < st->bf_sz);
        mem_move(st->bf, st->bf + st->bf_idx, st->bf_used - st->bf_idx);
        st->bf_used = st->bf_idx;
        st->bf_idx = 0;
        // Read new data
        uptr buffer_size_aviable = st->bf_sz - st->bf_used;
        uptr read_data_size = st->file_size - st->file_idx;
        if (read_data_size > buffer_size_aviable) {
            read_data_size = buffer_size_aviable;
        }
        uptr bytes_read = read_file(st->file, 0xFFFFFFFF, st->bf + st->bf_used, read_data_size);
        st->bf_used += bytes_read;
    }
}

static InStream stdin_stream_storage;
static OutStream stdout_stream_storage;
static OutStream stderr_stream_storage;

static InStream *stdin_stream;
static OutStream *stdout_stream;
static OutStream *stderr_stream;

InStream *get_stdin_stream(void) {
    if (stdin_stream == 0) {
        stdin_stream_storage = create_in_streamf(get_stdin_file(), STDIN_STREAM_BF_SZ, STDIN_STREAM_THRESHOLD, TRUE);
        stdin_stream = &stdin_stream_storage;
    }
    return stdin_stream;
}

OutStream *get_stdout_stream(void) {
    if (stdout_stream == 0) {
        stdout_stream_storage = create_out_streamf(get_stdout_file(), STDOUT_STREAM_BF_SZ, STDOUT_STREAM_THRESHOLD, TRUE);
        stdout_stream = &stdout_stream_storage;
    }
    return stdout_stream;
}

OutStream *get_stderr_stream(void) {
    if (stderr_stream == 0) {
        stderr_stream_storage = create_out_streamf(get_stderr_file(), STDOUT_STREAM_BF_SZ, STDOUT_STREAM_THRESHOLD, TRUE);
        stderr_stream = &stderr_stream_storage;
    } 
    return stderr_stream;
}