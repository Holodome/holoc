// Author: Holodome
// Date: 13.09.2021 
// File: pkby/src/lib/stream.h
// Version: 0
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
#pragma once
#include "lib/general.h"
#include "lib/strings.h"
#include "lib/filesystem.h"

#define OUT_STREAM_DEFAULT_BUFFER_SIZE KB(16)
// This is similar to stdlib's one, which is also usually 4kb
#define OUT_STREAM_DEFAULT_MAX_PRINTF_LEN KB(4)
#define OUT_STREAM_DEFAULT_THRESHOLD (OUT_STREAM_DEFAULT_BUFFER_SIZE - OUT_STREAM_DEFAULT_MAX_PRINTF_LEN)
#define IN_STREAM_DEFAULT_BUFFER_SIZE KB(8)
#define IN_STREAM_DEFAULT_MAX_CONSUMPTION_SIZE KB(1)
#define IN_STREAM_DEFAULT_THRESHLOD (IN_STREAM_DEFAULT_BUFFER_SIZE - IN_STREAM_DEFAULT_MAX_CONSUMPTION_SIZE)
#define STDIN_STREAM_BF_SZ KB(16)
#define STDIN_STREAM_THRESHOLD KB(12)
#define STDOUT_STREAM_BF_SZ KB(16)
#define STDOUT_STREAM_THRESHOLD KB(12)

enum {
    STREAM_BUFFER,
    STREAM_FILE,
    STREAM_ST, // stdin, stdout, stderr
};

// Stream is an object that supports continously writing to while having
// relatively stable write time perfomance.
// Streams are used to write to output files, but OS write calls are quite expnesive.
// That is why stream contains buffer that is written to until threshold is hit (to allow writes of arbitrary sizes)
// When threshold is hit a flush happens - buffer is written to output file and ready to recieve new input
// buffer_size - buffer_threshold define maximum size of single write 
typedef struct {
    u32 mode;
    
    OSFileHandle *file;
    uptr file_idx;
    
    u8 *bf;
    uptr bf_sz;
    uptr threshold;
    
    uptr bf_idx;
} OutStream;

// Create stream for writing to file.
// bf_sz - what size of buffer to allocate 
// threshold >= bf_sz
// bf - storage for stream buffer
void init_out_streamf(OutStream *st, OSFileHandle *file_handle,
    void *bf, uptr bf_sz, uptr threshold, b32 is_std);
// Printfs to stream
__attribute__((__format__ (__printf__, 2, 3)))
uptr out_streamf(OutStream *st, const char *fmt, ...);
uptr out_streamv(OutStream *st, const char *fmt, va_list args);
void out_stream_flush(OutStream *st);

// Threshold defines how much of additonal data is read between flushes.
// For example, buffer may be 6 kb and threshold 4kb. Then 
// each time additional 2 kb is read. This way stream can always read at least 2kb bytes
// This is useful in parsing text files, when, for example, single line have to be read.
// We can safely say that line is no longer than 2048 symbols (hopefully...).
// Then whole line can be read and parsed correclty, but in other cases only part
// of line will be read and then it is not clear what behavour of program should be.
// When threshold is reached, new bf_sz bytes are read, and bytes located afther threshold are moved
// at buffer start. So, the bigger the bf_sz - thrreshold, the more stream can read at once, 
// but the more time it spends on copying and moving around memory
// Way around this can be allowing buffer of growing size 
// @NOTE no flusing happens when in stream uses buffer to read from
typedef struct {
    u32 mode;
    
    OSFileHandle *file;
    uptr file_size;
    uptr file_idx;
    // Buffer in stream is used for caching read results
    u8 *bf;
    // Total size of buffer
    uptr bf_sz;
    // Current read index in buffer
    uptr bf_idx;
    // How many bytes the buffer has data of. Not equals to bf_sz only in cases where
    // chunk contained in bf is last in file
    uptr bf_used;
    // After what number bf_idx should be reset and buffer refilled
    uptr threshold;
    b32 is_finished;
} InStream;

void init_in_streamf(InStream *st, OSFileHandle *file, void *bf, uptr bf_sz, uptr threshold, b32 is_std);
// Peek next n bytes without advancing the cursor
// Returns number of bytes peeked
uptr in_stream_peek(InStream *st, void *out, uptr n);
// Advance stream by n bytes. 
// Return numbef of bytes advanced by
uptr in_stream_advance(InStream *st, uptr n);
// If stream is bufferized, read next file chunk to fill the buffer as much as possible
void in_stream_flush(InStream *st);
// Helper function. Used in parsin text, where only next one byte needs to be peeked to be checked
u8 in_stream_peek_b_or_zero(InStream *st);

OutStream *get_stdout_stream(void);
OutStream *get_stderr_stream(void);
