#include "strings.h"

#include <stdio.h>

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

void str_to_mantissa_exponent(const char *str, uptr len, 
        u32 *mantissa_out, u32 *exponent_out) {
    u32 integer_part = 0;
    u32 fraction_part = 0;
    u32 fraction_part_delimeter = 1;

    b32 dot_is_passed = FALSE;
    while (len--) {
        u32 symb = *str++;
        if (is_digit(symb)) {
            if (dot_is_passed) {
                fraction_part = fraction_part * 10 + (symb - '0');
                fraction_part_delimeter *= 10;
            } else {
                integer_part = integer_part * 10 + (symb - '0');
            }
        } else if (symb == '.') {
            dot_is_passed = TRUE;
        } else {
            break;
        }
    }

    if (integer_part || fraction_part) {
        u32 frac = fraction_part;
        u32 delim = fraction_part_delimeter;
        u32 frac_bin = 0;
#define MANTISSA_BITS 52 
        u32 mantissa_bits_count = 0;
        while (mantissa_bits_count++ < MANTISSA_BITS) {
            frac *= 2;
            frac_bin = (frac_bin << 1) | (frac / delim);
            frac %= delim;
            if (frac == 0) {
                break;
            }
        }
        printf("%x %x\n", frac_bin, integer_part);
    }
}

f64 str_to_f64(const char *str, uptr len) {
    f64 result = 0;
    return result;
} 
i64 str_to_i64(const char *str, uptr len);


