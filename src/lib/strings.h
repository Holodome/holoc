// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/lib/strings.h
// Version: 0
//
// Defines series of functions that are frequently used when oprating with strings.
#pragma once
#include "lib/general.h"

// snprintf wrappers
uptr vfmt(char *buf, uptr buf_size, const char *format, va_list args);
__attribute__((__format__ (__printf__, 3, 4)))
uptr fmt(char *buf, uptr buf_size, const char *format, ...);
      
bool is_ascii(u32 symb);
bool is_space(u32 symb);
bool is_nextline(u32 symb);
bool is_digit(u32 symb);
bool is_alpha(u32 symb);
bool is_ident(u32 symb);
bool is_ident_start(u32 symb);
bool is_punct(u32 symb);
bool is_real(u32 symb);
bool is_int(u32 symb);

f64 str_to_f64(const char *str);
i64 str_to_i64(const char *str);

// Copies string str to bf. Number of written bytes is min(bf_sz, strlen(str) + 1)
// Return number of copied bytes
uptr str_cp(char *bf, uptr bf_sz, const char *str);
// Returns true if two given strings are equal, false otherwise
bool str_eq(const char *a, const char *b);
// Returns true if first n characters of both strings are equal or strings are equal
bool str_eqn(const char *a, const char *b, uptr n);
// Returns string length
uptr str_len(const char *str);

// Returns length of utf8-encoded symbol. 
// 0 means error
u32 utf8_encode(u32 utf32, u8 *dst);
const void *utf8_decode(const void *buf, u32 *codepoint);
// Printf wrappers
uptr outf(const char *msg, ...);
uptr outv(const char *msg, va_list args);
uptr erroutf(const char *msg, ...);
uptr erroutv(const char *msg, va_list args);

// What is known a string usually. In some systems terminology between these terms differs,
// but here Text is synonymous to string.
// But cstrings should be called strings, and 'strings' are called text
typedef struct Text {
    const char *data;
    u32 len;
} Text;

Text text(const char *data, u32 len);
bool text_eq(Text a, Text b);
bool text_startswith(Text a, Text b);
bool text_endswith(Text a, Text b);
Text text_substr(Text a, u32 start, u32 end);

typedef struct Text_UTF8 {
    u8 *data;
    uptr len;
} Text_UTF8;