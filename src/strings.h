#pragma once
#include "general.h"

// snprintf wrappers
uptr vfmt(char *buf, uptr buf_size, const char *format, va_list args);
uptr fmt(char *buf, uptr buf_size, const char *format, ...);

b32 is_space(u32 symb);
b32 is_nextline(u32 symb);
b32 is_digit(u32 symb);
b32 is_alpha(u32 symb);

f64 str_to_f64(const char *str, uptr len);
i64 str_to_i64(const char *str, uptr len);

