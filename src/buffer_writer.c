#include "buffer_writer.h"

#include <stdio.h>

void
buf_writev(buffer_writer *w, char *fmt, va_list args) {
    uint32_t wrote = vsnprintf(w->cursor, w->eof - w->cursor, fmt, args);
    w->cursor += wrote;
}

void
buf_write(buffer_writer *w, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_writev(w, fmt, args);
}
