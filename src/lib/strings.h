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
uptr fmt(char *buf, uptr buf_size, const char *format, ...);

b32 is_ascii(u32 symb);
b32 is_space(u32 symb);
b32 is_nextline(u32 symb);
b32 is_digit(u32 symb);
b32 is_alpha(u32 symb);
b32 is_ident(u32 symb);
b32 is_ident_start(u32 symb);
b32 is_punct(u32 symb);

f64 str_to_f64(const char *str);
i64 str_to_i64(const char *str);

// Copies string str to bf. Number of written bytes is min(bf_sz, strlen(str) + 1)
// Return number of copied bytes
uptr str_cp(char *bf, uptr bf_sz, const char *str);
// Returns true if two given strings are equal, false otherwise
b32 str_eq(const char *a, const char *b);
// Returns true if first n characters of both strings are equal or strings are equal
b32 str_eqn(const char *a, const char *b, uptr n);
// Returns string length
uptr str_len(const char *str);

// Returns length of utf8-encoded symbol. 
// 0 means error
u32 utf8_encode(u32 utf32, u8 *dst);
// Returns utf32 codepoint of first symbol encoded in src. 
// length is written in len. If len equals 0, error happened
u32 utf8_decode(const u8 *src,u32 *len_out);
// Printf wrappers
uptr outf(const char *msg, ...);
uptr outv(const char *msg, va_list args);
uptr erroutf(const char *msg, ...);
uptr erroutv(const char *msg, va_list args);

// What is known a string usually. In some systems terminology between these terms differs,
// but here Text is synonymous to string.
// But cstrings should be called strings, and 'strings' are called text
typedef struct Text {
    char *data;
    uptr len;
} Text;

typedef struct TextUTF8 {
    u8 *data;
    uptr len;
} TextUTF8;