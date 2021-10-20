// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/lib/strings.h
// Version: 0
//
// Defines series of functions that are frequently used when oprating with strings.
#ifndef STRINGS_H
#define STRINGS_H
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

f64 z2f64(const char *str);
i64 z2i64(const char *str);

// Copies string str to bf. Number of written bytes is min(bf_sz, strlen(str) + 1)
// Return number of copied bytes
uptr zcp(char *bf, uptr bf_sz, const char *str);
// Returns true if two given strings are equal, false otherwise
bool zeq(const char *a, const char *b);
// Returns true if first n characters of both strings are equal or strings are equal
bool zeqn(const char *a, const char *b, uptr n);
// Returns string length
uptr zlen(const char *str);

// Returns length of utf8-encoded symbol. 
// 0 means error
u32 utf8_encode(u32 utf32, u8 *dst);
const void *utf8_decode(const void *buf, u32 *codepoint);
// Printf wrappers
uptr outf(const char *msg, ...);
uptr outv(const char *msg, va_list args);
uptr erroutf(const char *msg, ...);
uptr erroutv(const char *msg, va_list args);

typedef struct Str {
    const char *data;
    u32 len;
} Str;

#define WRAP_Z(_z) ((Str){ _z, sizeof(_z) - 1})
#define strz(_str) str(_str, zlen(_str))
Str str(const char *data, u32 len);
bool str_eq(Str a, Str b);
bool str_startswith(Str a, Str b);
bool str_endswith(Str a, Str b);
Str str_substr(Str a, u32 start, u32 end);
#define str_lstrip(_a, ...) str_lstrip_(_a, (WRAP_Z(" "), ##__VA_ARGS__))
Str str_lstrip_(Str a, Str chars);
#define str_rstrip(_a, ...) str_rstrip_(_a, (WRAP_Z(" "), ##__VA_ARGS__))
Str str_rstrip_(Str a, Str chars);
#define str_strip(_a, ...) str_strip_(_a, (WRAP_Z(" "), ##__VA_ARGS__))
Str str_strip_(Str a, Str chars);
u32 str_countc(Str str, char symb);
u32 str_count(Str str, Str substr);
u32 str_findc(Str a, char symb); 
u32 str_find(Str a, Str substr);
u32 str_rfindc(Str a, char symb); 
u32 str_rfind(Str a, Str substr);
void str_lower(Str *strp);
void str_upper(Str *strp);
void str_capitalize(Str *strp);

#endif