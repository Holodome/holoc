#include "lib/stream.h"
#include "lib/memory.h"
#include "lib/strings.h"

static Out_Stream stdout_stream_storage;
static Out_Stream stderr_stream_storage;

static Out_Stream *stdout_stream;
static Out_Stream *stderr_stream;

static u32 
calculate_threshold(u32 buf_size) {
    u32 result = buf_size * 3 / 4;
    u32 max_threshold = KB(4); 
    if (result > max_threshold) {
        result = max_threshold;
    }
    return result;
}

Out_Stream *get_stdout_stream(void) {
    static u8 stdout_buffer[OUT_STREAM_DEFAULT_BUFFER_SIZE];
    if (stdout_stream == 0) {
        stdout_stream_storage.first_chunk.buf = stdout_buffer;
        stdout_stream_storage.buf_size = sizeof(stdout_buffer);
        stdout_stream_storage.mode = STREAM_STDOUT;
        stderr_stream_storage.threshold = calculate_threshold(sizeof(stdout_buffer));
        stdout_stream = &stdout_stream_storage;
    }
    return stdout_stream;
}

Out_Stream *get_stderr_stream(void) {
    static u8 stderr_buffer[OUT_STREAM_DEFAULT_BUFFER_SIZE];
    if (stderr_stream == 0) {
        stderr_stream_storage.first_chunk.buf = stderr_buffer;
        stderr_stream_storage.buf_size = sizeof(stderr_buffer);
        stderr_stream_storage.threshold = calculate_threshold(sizeof(stderr_buffer));
        stderr_stream_storage.mode = STREAM_STDERR;
        stderr_stream = &stderr_stream_storage;
    } 
    return stderr_stream;
}

Out_Stream 
init_out_streamf(OS_File_Handle file_handle, void *buf, u32 buf_size) {
    Out_Stream stream = {0};
    stream.mode = STREAM_FILE;
    stream.file = file_handle;
    stream.first_chunk.buf = buf;
    stream.buf_size = buf_size;
    u32 threshold = calculate_threshold(buf_size);
    stream.threshold = threshold;
    return stream;
}

Out_Stream
out_stream_on_demand(struct Memory_Arena *arena, u32 chunk_size) {
    Out_Stream stream = {0};
    stream.mode = STREAM_ON_DEMAND;
    stream.arena = arena;
    stream.buf_size = chunk_size;
    u32 threshold = calculate_threshold(chunk_size);
    stream.threshold = threshold;    
    return stream;
}

Out_Stream
out_stream_buffer(void *buffer, u32 buffer_size) {
    Out_Stream stream = {0};
    stream.mode = STREAM_BUFFER;
    stream.first_chunk.buf = buffer;
    stream.buf_size = buffer_size;
    return stream;
}

u32 
out_streamv(Out_Stream *stream, const char *format, va_list args) {
    if (stream->first_chunk.buf_cursor > stream->threshold) {
        out_stream_flush(stream);
    }
    
    u32 result = vfmt((char *)stream->first_chunk.buf + stream->first_chunk.buf_cursor, 
        stream->buf_size - stream->first_chunk.buf_cursor, format, args);
    stream->first_chunk.buf_cursor += result;
    return result;
}

u32 
out_streamf(Out_Stream *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    return out_streamv(stream, format, args);
}

void 
out_stream_flush(Out_Stream *stream) {
    switch (stream->mode) {
    case STREAM_BUFFER: {
    } break;
    case STREAM_ON_DEMAND: {
        Out_Stream_Chunk *new_chunk = arena_alloc_struct(stream->arena, Out_Stream_Chunk);
        mem_copy(new_chunk, &stream->first_chunk, sizeof(stream->first_chunk));
        mem_zero(&stream->first_chunk, sizeof(stream->first_chunk));
        stream->first_chunk.next = new_chunk;
        stream->first_chunk.buf = arena_alloc(stream->arena, stream->buf_size);
    } break;
    case STREAM_FILE: {
        os_write_file(stream->file, stream->file_offset, stream->first_chunk.buf, stream->first_chunk.buf_cursor);
        stream->first_chunk.buf_cursor = 0;
    } break;
    case STREAM_STDERR: {
        os_write_stderr(stream->first_chunk.buf, stream->first_chunk.buf_cursor);
        stream->first_chunk.buf_cursor = 0;
    } break;
    case STREAM_STDOUT: {
        os_write_stdout(stream->first_chunk.buf, stream->first_chunk.buf_cursor);
        stream->first_chunk.buf_cursor = 0;
    } break;
    }    
}
