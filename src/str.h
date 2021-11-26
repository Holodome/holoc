/*
Author: Holodome
Date: 18.11.2021
File: src/string.h
Version: 0
*/
#ifndef STRING_H
#define STRING_H

#include "types.h"

uint32_t fmtv(char *buf, uint32_t buf_size, char *format, va_list args);
__attribute__((__format__(__printf__, 3, 4)))
uint32_t fmt(char *buf, uint32_t buf_size, char *format, ...);

bool is_ident_start(uint32_t symb);
bool is_ident(uint32_t symb);

#ifdef STR_CREATED_AT_INFO
#define STR_CREATED_AT__(_file, _line) _file _line 
#define STR_CREATED_AT_(_file, _line) STR_CREATED_AT__(_file, #_line)
#define STR_CREATED_AT       , STR_CREATED_AT_(__FILE__, __LINE__)
#define STR_CREATED_AT_PARAM , char *created_at
#define STR_CREATED_AT_PASS  , created_at
#else 
#define string_wrap_call(_func, ...) _func(__VA_ARGS__)
#define STR_CREATED_AT
#define STR_CREATED_AT_PARAM 
#define STR_CREATED_AT_PASS  
#endif // STR_CREATED_AT_INFO

#define string(_data, _len) string_(_data, _len STR_CREATED_AT)
string string_(char *_data, uint32_t len STR_CREATED_AT_PARAM);
#define WRAP_Z(_z)       string(_z, sizeof(_z) - 1)
#define stringz(_string) string(_string, strlen(_string))

bool string_eq(string a, string b);
bool string_startswith(string a, string b);
bool string_endswith(string a, string b);
#define string_substr(_a, _start, _end)  string_substr(_a, _start, _end STR_CREATED_AT)
string string_substr_(string a, uint32_t start, uint32_t end STR_CREATED_AT_PARAM);
#define string_lstrip(_a, _start, _end)  string_lstrip_(str, symbs STR_CREATED_AT)
string string_lstrip_(string str, string symbs STR_CREATED_AT_PARAM);
#define string_rstrip(_a, _start, _end)  string_rstrip_(str, symbs STR_CREATED_AT)
string string_rstrip_(string str, string symbs STR_CREATED_AT_PARAM);
#define string_strip(_a, _start, _end)  string_strip_(str, symbs STR_CREATED_AT)
string string_strip_(string str, string symbs STR_CREATED_AT_PARAM);

#endif