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
// 3: Buffers. User-defned buffers
// 
// Use of streams allows writing all input and output code in same manner, generally improving code
// reuse.
// Streams are similar to FILE * objects from c standard library.
//
// @NOTE Input and output streams are split into two different data structures.
// This is done to preserve clarity in code for reading - because single use case can never mix 
// reading and writing to same locaiton
#ifndef STREAM_H
#define STREAM_H
#include "lib/general.h"
#include "lib/files.h"

#define OUT_STREAM_DEFAULT_BUFFER_SIZE KB(16)
// This is similar to stdlib's one, which is also usually 4kb
#define OUT_STREAM_DEFAULT_MAX_PRINTF_LEN KB(4)
#define OUT_STREAM_DEFAULT_THRESHOLD (OUT_STREAM_DEFAULT_BUFFER_SIZE - OUT_STREAM_DEFAULT_MAX_PRINTF_LEN)
#define STDOUT_STREAM_BF_SZ KB(16)
#define STDOUT_STREAM_THRESHOLD KB(12)
#define IN_STREAM_DEFAULT_BUFFER_SIZE OUT_STREAM_DEFAULT_BUFFER_SIZE
#define IN_STREAM_DEFAULT_THRESHLOD OUT_STREAM_DEFAULT_THRESHOLD

// @NOTE(hl): Simplest kind of stream - just plan data pointer with no memory handling
typedef struct {
    u8 *data;
    u64 data_size;
    u64 initial_data_size;
} Buffer;

void buffer_advance(Buffer *buffer, u64 n);

enum {
    STREAM_BUFFER,
    STREAM_FILE,
    STREAM_STDOUT,
    STREAM_STDERR
};

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
    u32 mode;
    
    OS_File_Handle file;
    uptr file_idx;
    
    u8 *bf;
    uptr bf_sz;
    uptr threshold;
    
    uptr bf_idx;
} Out_Stream;

// Create stream for writing to file.
// bf_sz - what size of buffer to allocate 
// threshold >= bf_sz
// bf - storage for stream buffer
void init_out_streamf(Out_Stream *stream, OS_File_Handle file_handle,
    void *bf, uptr bf_sz, uptr threshold);
// Printfs to stream
__attribute__((__format__ (__printf__, 2, 3)))
uptr out_streamf(Out_Stream *stream, const char *fmt, ...);
uptr out_streamv(Out_Stream *stream, const char *fmt, va_list args);
void out_stream_flush(Out_Stream *stream);

Out_Stream *get_stdout_stream(void);
Out_Stream *get_stderr_stream(void);

// @NOTE(hl): Acceleration structure for handling text input 
//  to avoid copying, provides direct memory access in size-constrained mode to the data
//  contained in file 
// @NOTE(hl): There is no plan to add support for stdin streams, just because there is no such necessity.
// typedef struct In_Stream {
//     OS_File_Handle file;
//     u64 file_cursor;
//     u64 file_size;
    
//     u8 *bf;
//     u32 bf_sz;
//     u32 threshold;
//     u32 bf_used;
// } In_Stream;

// void init_in_stream(In_Stream *stream, OS_File_Handle file_handle, 
//     void *bf, u32 bf_sz, u32 threshold);
// Buffer in_stream_get_data(In_Stream *stream);
// void in_stream_advance(In_Stream *stream);

// @NOTE(hl): Acceleration structure for handling UTF8 input.
//  decodes a part of text and stores unicode codepoints in full size
// typedef struct In_UTF8_Stream {
//     In_Stream *in_stream;
//     u32 *unicode_codepoint_buf;
//     u32  unicode_codepoint_buf_size;
//     u32  unicode_codepoint_buf_threshold;
// } In_UTF8_Stream;

// void init_in_utf8_stream(In_UTF8_Stream *stream, In_Stream *raw_stream, 
//     void *bf, u32 bf_sz, 
//     u32 threshold);
// void in_utf8_stream_

#endif