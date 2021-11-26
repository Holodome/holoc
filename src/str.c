#include "str.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "my_assert.h"

uint32_t 
fmtv(char *buf, uint32_t buf_size, char *format, va_list args) {
    return vsnprintf(buf, buf_size, format, args);
}

uint32_t 
fmt(char *buf, uint32_t buf_size, char *format, ...) {
    va_list args;
    va_start(args, format);
    uint32_t result = fmtv(buf, buf_size, format, args);
    va_end(args);
    return result;
}

bool 
is_ident_start(uint32_t symb) {
    return isalpha(symb) || symb == ' ';
}

bool 
is_ident(uint32_t symb) {
    return isalnum(symb) || symb == ' ';
}

string 
string_(char *data, uint32_t len STR_CREATED_AT_PARAM) {
    return (string) { data, len STR_CREATED_AT_PASS };    
}

bool 
string_eq(string a, string b) {
    return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

bool 
string_startswith(string a, string b) {
    return a.len >= b.len && memcmp(a.data, b.data, b.len) == 0;
}

bool 
string_endswith(string a, string b) {
    return a.len >= b.len && memcmp(a.data + a.len - b.len, b.data, b.len) == 0;
}

string 
string_substr_(string a, uint32_t start, uint32_t end STR_CREATED_AT_PARAM) {
    assert(a.len >= end && a.len >= start && end >= start);
    return string_(a.data + start, end - start STR_CREATED_AT_PASS);
}

string 
string_lstrip_(string a, string symbs STR_CREATED_AT_PARAM) {
    for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test = a.data[0];
        for (uint32_t i = 0; i < symbs.len; ++i) {
            if (symbs.data[i] == test) {
                is_found = true;
                break;
            }
        }
        
        if (is_found) {
            a = string_substr_(a, 1, a.len STR_CREATED_AT_PASS);
        } else {
            break;
        }
    }
    return a;
}

string 
string_rstrip_(string a, string symbs STR_CREATED_AT_PARAM) {
    for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test = a.data[a.len - 1];
        for (uint32_t i = 0; i < symbs.len; ++i) {
            if (symbs.data[i] == test) {
                is_found = true;
                break;
            }
        }
        
        if (is_found) {
            a = string_substr_(a, 0, a.len - 1 STR_CREATED_AT_PASS);
        } else {
            break;
        }
    }
    return a;
}

string 
string_strip_(string str, string symbs STR_CREATED_AT_PARAM) {
    return string_lstrip_(string_rstrip_(str, symbs STR_CREATED_AT_PASS), symbs STR_CREATED_AT_PASS);   
}
