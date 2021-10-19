// Author: Holodome
// Date: 13.09.2021 
// File: pkby/src/lib/stream.h
// Version: 2
// 
// Defines series of data types that represent differnt types of input and output streams
// Stream is an object that provides API to continoiusly access data from different sources
// In particular, streams can be used to access 3 different type of data sources:
// 1: Files. Input and output streams support delayed writing, improving perfomance over calling os
// write call every time
// 2: Standard streams. These are stdin, stdout, stderr
// 3: Buffers. 
#ifndef STREAM_H
#define STREAM_H

#include "lib/general.h"
#include "lib/files.h"

struct Memory_Arena;

#define OUT_STREAM_DEFAULT_BUFFER_SIZE    KB(16)

enum {
    STREAM_ON_DEMAND = 0x0,
    STREAM_BUFFER    = 0x1,
    STREAM_FILE      = 0x2,
    STREAM_STDOUT    = 0x3,
    STREAM_STDERR    = 0x4,
};

typedef struct Out_Stream_Chunk {
    void *buf;
    u32   buf_cursor;
    
    struct Out_Stream_Chunk *next;
} Out_Stream_Chunk;


/*
Stream is an object that supports continously writing to while having
relatively stable write time perfomance.
Streams are used to write to output files, but OS write calls are quite expnesive.
That is why stream contains buffer that is written to until threshold is hit (to allow writes of arbitrary sizes)
When threshold is hit a flush happens - buffer is written to output file and ready to recieve new input
buffer_size - buffer_threshold define maximum size of single write 

@NOTE(hl): THIS IS NOT HIGH-PERFOMANCE STRUCTURE.
 The core principle of this is to unify differnt output methods, and this involves 
 a lot of last-minute decision making in functions' implementations.
 If one needs fast way to output data, they should use the one method they need
*/
typedef struct Out_Stream {
    u8 mode;
    union {
        struct {
            OS_File_Handle file;
            u32            file_offset;
        };
        // For on-demand memory stream.
        struct {
            struct Memory_Arena *arena; 
        };
    };
    
    Out_Stream_Chunk first_chunk;
    u32 buf_size;
    u32 threshold;
} Out_Stream;

Out_Stream *get_stdout_stream(void);
Out_Stream *get_stderr_stream(void);
#define STDOUT get_stdout_stream()
#define STDERR get_stderr_stream()

Out_Stream out_stream(OS_File_Handle file_handle, void *buf, u32 buf_size);
Out_Stream out_stream_on_demand(struct Memory_Arena *arena, u32 chunk_size);
Out_Stream out_stream_buffer(void *buffer, u32 buffer_size);

u32 out_streamv(Out_Stream *stream, const char *fmt, va_list args);
__attribute__((__format__ (__printf__, 2, 3)))
u32 out_streamf(Out_Stream *stream, const char *fmt, ...);
#define out_streamfln(_stream, _fmt, ...) out_streamf(_stream, _fmt "\n", ##__VA_ARGS__)
// u32 out_streamb(Out_Stream *stream, const void *data, u32 data_size);
void out_stream_flush(Out_Stream *stream);

#endif