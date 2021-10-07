#include "lib/stream.h"
#include "platform/memory.h"
#include "lib/strings.h"

static bool
out_st_needs_flush(Out_Stream *stream) {
    bool result = false;
    if (stream->mode == STREAM_FILE || stream->mode == STREAM_STDERR || stream->mode == STREAM_STDOUT) {
        result = stream->bf_idx >= stream->threshold;
    }
    return result;
}
// When workign with binary data and size of sizngle data block is bigger than
// threshold, data should be written directly
static void out_stream_write_direct(Out_Stream *stream, const void *data, uptr data_sz) {
    if (stream->mode == STREAM_FILE) {
        uptr written = os_write_file(stream->file, stream->file_idx, data, data_sz);
        stream->file_idx += written;
    }
}

void init_out_stream(Out_Stream *stream, void *bf, uptr bf_sz) {
    stream->mode = STREAM_BUFFER;
    stream->bf = bf;
    stream->bf_sz = bf_sz;
}

void init_out_streamf(Out_Stream *stream, OS_File_Handle *file,  void *bf, uptr bf_sz, uptr threshold) {
    assert(threshold < bf_sz);
    stream->file = file;
    stream->mode = STREAM_FILE;
    stream->bf = bf;
    stream->bf_sz = bf_sz;
    stream->threshold = threshold;
}

uptr out_streamf(Out_Stream *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    return out_streamv(stream, format, args);
}

uptr out_streamv(Out_Stream *stream, const char *format, va_list args) {
    assert(!out_st_needs_flush(stream));
    u32 bytes_written = vfmt((char *)stream->bf + stream->bf_idx, stream->bf_sz - stream->bf_idx, format, args);
    stream->bf_idx += bytes_written;
    // this should never be hit due to nature of fmt, but assert in case fmt breaks
    assert(stream->bf_idx < stream->bf_sz);
    if (out_st_needs_flush(stream)) {
        out_stream_flush(stream);
    }
    return bytes_written;   
}

void out_streamb(Out_Stream *stream, const void *b, uptr c) {
    // decide writing policy 
    if (c <= stream->bf_sz) {
        if (stream->bf_idx + c >= stream->bf_sz) {
            out_stream_flush(stream);
        }
        mem_copy(stream->bf, b, c);
        stream->bf_idx = c;
        if (out_st_needs_flush(stream)) {
            out_stream_flush(stream);
        }
    } else {
        out_stream_flush(stream);
        out_stream_write_direct(stream, b, c);
    }
}

void out_stream_flush(Out_Stream *stream) {
    if (stream->mode == STREAM_BUFFER) {
        // nop  
    } else if (stream->mode == STREAM_FILE) {
        out_stream_write_direct(stream, stream->bf, stream->bf_idx);
        stream->bf_idx = 0;
    } else if (stream->mode == STREAM_STDOUT) {
        os_write_stdout(stream->bf, stream->bf_idx);
        stream->bf_idx = 0;
    } else if (stream->mode == STREAM_STDERR) {
        os_write_stderr(stream->bf, stream->bf_idx);
        stream->bf_idx = 0;
    }
}

void init_in_stream(In_Stream *stream, void *bf, uptr bf_sz) {
    stream->mode = STREAM_BUFFER;
    stream->bf = bf;
    stream->bf_sz = bf_sz;
}

void init_in_streamf(In_Stream *stream, OS_File_Handle *file, void *bf, uptr bf_sz, uptr threshold) {
    assert(bf_sz > threshold);
    stream->file = file;
    stream->file_size = os_get_file_size(file);
    stream->mode = STREAM_FILE;
    stream->bf = bf;
    stream->bf_sz = bf_sz;
    stream->threshold = threshold;
}

uptr in_stream_peek(In_Stream *stream, void *out, uptr n) {
    uptr result = 0;
    if (!stream->is_finished) {
        if (stream->bf_idx + n > stream->bf_used) {
            in_stream_flush(stream);
        }
        
        if (stream->bf_idx + n <= stream->bf_used) {
            mem_copy(out, stream->bf + stream->bf_idx, n);
            result = n;
        } else {
            NOT_IMPLEMENTED;
        }
    }      
    return result;
}

u8 in_stream_soft_peek_at(In_Stream *stream, uptr offset) {
    u8 result = 0;
    if (stream->bf_idx + offset <= stream->bf_used) {
        result = stream->bf[stream->bf_idx + offset];
    }
    return result;
}

uptr in_stream_advance(In_Stream *stream, uptr n) {
    uptr result = 0;
    if (!stream->is_finished) {
        if (stream->bf_idx + n > stream->bf_used) {
            in_stream_flush(stream);
        }
        
        if (stream->bf_idx + n <= stream->bf_used) {
            stream->bf_idx += n;
            result += n;
            
            if (stream->bf_idx > stream->threshold) {
                in_stream_flush(stream);
            }
        } else {
            NOT_IMPLEMENTED;
        }
    }
    return result;
}

void in_stream_flush(In_Stream *stream) {
    if (stream->mode == STREAM_BUFFER) {
        // nop
    } else if (stream->mode == STREAM_FILE) {
        // Move chunk of file that is not processed to buffer start
        assert(stream->bf_idx < stream->bf_sz);
        mem_move(stream->bf, stream->bf + stream->bf_idx, stream->bf_used - stream->bf_idx);
        stream->bf_used = stream->bf_idx;
        stream->bf_idx = 0;
        // Read new data
        uptr buffer_size_aviable = stream->bf_sz - stream->bf_used;
        uptr read_data_size = stream->file_size - stream->file_idx;
        if (read_data_size > buffer_size_aviable) {
            read_data_size = buffer_size_aviable;
        }
        uptr bytes_read = os_read_file(stream->file, stream->file_idx, stream->bf + stream->bf_used, read_data_size);
        stream->bf_used += bytes_read;
        stream->file_idx += bytes_read;
        if (bytes_read == 0) {
            // @TODO buggy
            stream->is_finished = true;
        }
    } 
}

u8 in_stream_peek_b_or_zero(In_Stream *stream) {
    u8 result = 0;
    in_stream_peek(stream, &result, 1);
    return result;    
}

static Out_Stream stdout_stream_storage;
static Out_Stream stderr_stream_storage;

static Out_Stream *stdout_stream;
static Out_Stream *stderr_stream;

Out_Stream *get_stdout_stream(void) {
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

Out_Stream *get_stderr_stream(void) {
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
