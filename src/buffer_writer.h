#ifndef BUFFER_WRITER_H
#define BUFFER_WRITER_H

#include "types.h"

typedef struct buffer_writer {
    char *cursor;
    char *eof;
} buffer_writer;

void buf_writev(buffer_writer *w, char *fmt, va_list args);

__attribute__((format(printf, 2, 3))) void buf_write(buffer_writer *w,
                                                     char *fmt, ...);

// Writes strings wiht replacing special characters as escape sequences
// and doing decoding.
void buf_write_raw_utf8(buffer_writer *w, void *str);
void buf_write_raw_utf16(buffer_writer *w, void *str);
void buf_write_raw_utf32(buffer_writer *w, void *str);

#endif
