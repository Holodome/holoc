/*
Author: Holodome
Date: 19.11.2021
File: src/out_stream.h
Version: 0
*/
#ifndef OUT_STREAM_H
#define OUT_STREAM_H 1

#include <stdint.h>

struct FILE;

typedef enum {
    OUT_STREAM_ON_DEMAND = 0x0,
    OUT_STREAM_BUFFER    = 0x1,
    OUT_STREM_FILE       = 0x2,
} out_stream_kind;

typedef struct out_stream_chunk {
    void    *buf;
    uint32_t buf_cursor;    
    struct out_stream_chunk *next;
} out_stream_chunk;

typedef struct out_stream {
    out_stream_kind kind;
    struct FILE *file;
    
} out_stream;

#endif 