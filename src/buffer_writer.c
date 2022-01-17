#include "buffer_writer.h"

#include <stdio.h>

#include "unicode.h"

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

static void
write_cp(buffer_writer *w, uint32_t cp) {
    switch (cp) {
    default: {
        char buf[5];
        buf[(char *)utf8_encode(buf, cp) - buf] = 0;
        buf_write(w, "%s", buf);
    } break;
    case '\a':
        buf_write(w, "\\a");
        break;
    case '\b':
        buf_write(w, "\\b");
        break;
    case '\f':
        buf_write(w, "\\f");
        break;
    case '\n':
        buf_write(w, "\\n");
        break;
    case '\r':
        buf_write(w, "\\r");
        break;
    case '\t':
        buf_write(w, "\\t");
        break;
    case '\v':
        buf_write(w, "\\v");
        break;
    }
}

void
buf_write_raw_utf8(buffer_writer *w, void *strv) {
    uint8_t *str = strv;
    while (*str) {
        uint32_t cp;
        str = utf8_decode(str, &cp);
        write_cp(w, cp);
    }
}

void
buf_write_raw_utf16(buffer_writer *w, void *strv) {
    uint16_t *str = strv;
    while (*str) {
        uint32_t cp;
        str = utf16_decode(str, &cp);
        write_cp(w, cp);
    }
}

void
buf_write_raw_utf32(buffer_writer *w, void *strv) {
    uint32_t *str = strv;
    while (*str) {
        uint32_t cp = *str++;
        write_cp(w, cp);
    }
}

