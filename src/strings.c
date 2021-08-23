#include "strings.h"

#include <stdio.h>
#define IEEE754_CONVERT_IMPLEMENTATION 
#include "ieee754_convert.h"

uptr vfmt(char *buf, uptr buf_size, const char *format, va_list args) {
    return vsnprintf(buf, buf_size, format, args);
}

uptr fmt(char *buf, uptr buf_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    uptr result = vfmt(buf, buf_size, format, args);
    va_end(args);
    return result;
}

b32 is_space(u32 symb) {
    return symb == ' ';
}

b32 is_nextline(u32 symb) {
    return (symb == '\n' || symb == '\r');  
}

b32 is_digit(u32 symb) {
    return ('0' <= symb && symb <= '9');
}

b32 is_alpha(u32 symb) {
    return ('a' <= symb && symb <= 'z') || ('A' <= symb && symb <= 'Z');
}

f64 str_to_f64(const char *str, uptr len) {
    return ieee754_convert_f64(str, len);
}

i64 str_to_i64(const char *str, uptr len) {
    i64 result = 0;
    b32 is_negative = FALSE;
    while (len--) {
        char symb = *str++;
        if (symb == '-') {
            is_negative = TRUE;
        } else if (symb == '+') {
        } else if ('0' <= symb && symb <= '9') {
            result = result * 10 + (symb - '0');
        } else {
            break;
        }
    }
    return result;
}
