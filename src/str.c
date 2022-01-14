#include "str.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"

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
string_substr_(string a, uint32_t start, uint32_t end) {
    assert(a.len >= end && a.len >= start && end >= start);
    return string(a.data + start, end - start);
}

string
string_lstrip_(string a, string symbs) {
    for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test     = a.data[0];
        for (uint32_t i = 0; i < symbs.len; ++i) {
            if (symbs.data[i] == test) {
                is_found = true;
                break;
            }
        }

        if (is_found) {
            a = string_substr_(a, 1, a.len);
        } else {
            break;
        }
    }
    return a;
}

string
string_rstrip_(string a, string symbs) {
    for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test     = a.data[a.len - 1];
        for (uint32_t i = 0; i < symbs.len; ++i) {
            if (symbs.data[i] == test) {
                is_found = true;
                break;
            }
        }

        if (is_found) {
            a = string_substr_(a, 0, a.len - 1);
        } else {
            break;
        }
    }
    return a;
}

string
string_strip_(string str, string symbs) {
    return string_lstrip_(string_rstrip_(str, symbs), symbs);
}

string_find_result
string_find(string str, char symb) {
    string_find_result result = {0};

    char *memchr_ptr = memchr(str.data, symb, str.len);
    if (memchr_ptr) {
        result.idx      = memchr_ptr - str.data;
        result.is_found = true;
    }
    return result;
}

string_find_result
string_rfind(string str, char symb) {
    string_find_result result = {0};
    if (str.len) {
        for (uint32_t idx1 = str.len; idx1; --idx1) {
            if (str.data[idx1 - 1] == symb) {
                result.is_found = true;
                result.idx      = idx1 - 1;
            }
        }
    }
    return result;
}

string
string_substr(string a, uint32_t start, uint32_t end) {
    assert(start < a.len && end < a.len);
    assert(end >= start);
    return string(a.data + start, end - start);
}

string
string_memprintf(allocator *a, char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    uint32_t len  = strlen(buffer);
    char *str_mem = aalloc(a, len + 1);
    memcpy(str_mem, buffer, len);
    str_mem[len] = 0;
    return stringz(str_mem);
}

string
string_memdup(allocator *a, char *data) {
    uint32_t len  = strlen(data);
    char *str_mem = aalloc(a, len + 1);
    memcpy(str_mem, data, len);
    str_mem[len] = 0;
    return stringz(str_mem);
}

string
string_dup(struct allocator *a, string str) {
    char *new_mem = aalloc(a, str.len + 1);
    memcpy(new_mem, str.data, str.len);
    new_mem[str.len] = 0;
    return string(new_mem, str.len);
}
