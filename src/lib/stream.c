#include "lib/stream.h"
#include "lib/memory.h"
#include "lib/strings.h"
#define STREAM_NEEDS_FLUSH(_st) ((_st)->mode == STREAM_FILE && (_st)->bf_idx >= (_st)->threshold)

// When workign with binary data and size of sizngle data block is bigger than
// threshold, data should be written directly
static void out_stream_write_direct(OutStream *st, const void *data, uptr data_sz) {
    if (st->mode == STREAM_FILE) {
        uptr written = os_write_file(st->file, st->file_idx, data, data_sz);
        st->file_idx += written;
    }
}

void init_out_stream(OutStream *st, void *bf, uptr bf_sz) {
    st->mode = STREAM_BUFFER;
    st->bf = bf;
    st->bf_sz = bf_sz;
}

void init_out_streamf(OutStream *st, OSFileHandle *file,  void *bf, uptr bf_sz, uptr threshold) {
    assert(threshold < bf_sz);
    st->file = file;
    st->mode = STREAM_FILE;
    st->bf = bf;
    st->bf_sz = bf_sz;
    st->threshold = threshold;
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
    } else if (st->mode == STREAM_STDOUT) {
        os_write_stdout(st->bf, st->bf_idx);
        st->bf_idx = 0;
    } else if (st->mode == STREAM_STDERR) {
        os_write_stderr(st->bf, st->bf_idx);
        st->bf_idx = 0;
    }
}

void init_in_stream(InStream *st, void *bf, uptr bf_sz) {
    st->mode = STREAM_BUFFER;
    st->bf = bf;
    st->bf_sz = bf_sz;
}

void init_in_streamf(InStream *st, OSFileHandle *file, void *bf, uptr bf_sz, uptr threshold) {
    assert(bf_sz > threshold);
    st->file = file;
    st->file_size = os_get_file_size(file);
    st->mode = STREAM_FILE;
    st->bf = bf;
    st->bf_sz = bf_sz;
    st->threshold = threshold;
}

uptr in_stream_peek(InStream *st, void *out, uptr n) {
    uptr result = 0;
    if (!st->is_finished) {
        if (st->bf_idx + n > st->bf_used) {
            in_stream_flush(st);
        }
        
        if (st->bf_idx + n <= st->bf_used) {
            mem_copy(out, st->bf + st->bf_idx, n);
            result = n;
        } else {
            NOT_IMPLEMENTED;
        }
    }      
    return result;
}

uptr in_stream_advance(InStream *st, uptr n) {
    uptr result = 0;
    if (!st->is_finished) {
        if (st->bf_idx + n > st->bf_used) {
            in_stream_flush(st);
        }
        
        if (st->bf_idx + n <= st->bf_used) {
            st->bf_idx += n;
            result += n;
            
            if (st->bf_idx > st->threshold) {
                in_stream_flush(st);
            }
        } else {
            NOT_IMPLEMENTED;
        }
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
        uptr bytes_read = os_read_file(st->file, st->file_idx, st->bf + st->bf_used, read_data_size);
        st->bf_used += bytes_read;
        st->file_idx += bytes_read;
        if (bytes_read == 0) {
            // @TODO buggy
            st->is_finished = TRUE;
        }
    } 
}

u8 in_stream_peek_b_or_zero(InStream *st) {
    u8 result = 0;
    in_stream_peek(st, &result, 1);
    return result;    
}

static OutStream stdout_stream_storage;
static OutStream stderr_stream_storage;

static OutStream *stdout_stream;
static OutStream *stderr_stream;

OutStream *get_stdout_stream(void) {
    static u8 stdout_buffer[STDOUT_STREAM_BF_SZ];
    if (stdout_stream == 0) {
        stdout_stream_storage.bf = stdout_buffer;
        stdout_stream_storage.bf_sz = STDOUT_STREAM_BF_SZ;
        stdout_stream_storage.threshold = STDOUT_STREAM_THRESHOLD;
        stdout_stream_storage.mode = STREAM_STDOUT;
        stdout_stream = &stdout_stream_storage;
    }
    return stdout_stream;
}

OutStream *get_stderr_stream(void) {
    static u8 stderr_buffer[STDOUT_STREAM_BF_SZ];
    if (stderr_stream == 0) {
        stderr_stream_storage.bf = stderr_buffer;
        stderr_stream_storage.bf_sz = STDOUT_STREAM_BF_SZ;
        stderr_stream_storage.threshold = STDOUT_STREAM_THRESHOLD;
        stderr_stream_storage.mode = STREAM_STDERR;
        stderr_stream = &stderr_stream_storage;
    } 
    return stderr_stream;
}
