#pragma once
#include "general.h"

// snprintf wrappers
uptr vfmt(char *buf, uptr buf_size, const char *format, va_list args);
uptr fmt(char *buf, uptr buf_size, const char *format, ...);

b32 is_ascii(u32 symb);
b32 is_space(u32 symb);
b32 is_nextline(u32 symb);
b32 is_digit(u32 symb);
b32 is_alpha(u32 symb);
b32 is_ident(u32 symb);
b32 is_punct(u32 symb);

f64 str_to_f64(const char *str, uptr len);
i64 str_to_i64(const char *str, uptr len);

b32 str_eq(const char *a, const char *b);
b32 str_eqn(const char *a, const char *b, uptr n);
uptr str_len(const char *str);

// Returns length of utf8-encoded symbol. 
// 0 means error
u32 utf8_encode(u32 utf32, u8 *dst);
// Returns utf32 codepoint of first symbol encoded in src. 
// length is written in len. If len equals 0, error happened
u32 utf8_decode(const u8 *src,u32 *len_out);
// Printf wrappers
uptr outf(const char *msg, ...);
uptr voutf(const char *msg, va_list args);
uptr erroutf(const char *msg, ...);
uptr verroutf(const char *msg, va_list args);