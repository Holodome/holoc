/*
Author: Holodome
Date: 18.11.2021
File: src/string.h
Version: 0
*/
#ifndef STRING_H
#define STRING_H

#include "general.h"

struct allocator;

typedef struct {
    bool is_found;
    uint32_t idx;
} string_find_result;

// Wrapper for string creation
#define string(_data, _len) ((string){.data = _data, .len = _len})
// Construct string from string literal
// TODO: this fails in gcc because compound literals are not considered const by it.
// What we can do here is lose the (string) part, and leave { _data, _len } when initializing.
// But that is another form of initialization that would required additional macro, so let's
// not do it for now and stick with clang.
#define WRAPZ(_z) string(_z, sizeof(_z) - 1)
// Construct string from null-terminated string
#define stringz(_string) string(_string, strlen(_string))

// Return true if two strings are equal
bool string_eq(string a, string b);
// Return true is a starts with b
bool string_startswith(string a, string b);
// Return true if a ends with b
bool string_endswith(string a, string b);
// Return first index of symb in str
string_find_result string_find(string str, char symb);
// Return last index of symb in str
string_find_result string_rfind(string str, char symb);
// Create substring from given string
string string_substr(string a, uint32_t start, uint32_t end);
// python-like lstrip for string
string string_lstrip(string str, string symbs);
// python-like rstrip for string
string string_rstrip(string str, string symbs);
// python-like strip for string
string string_strip(string str, string symbs);

// Does printf and allocates that string.
string string_memprintf(struct allocator *a, char *format, ...);
// Works similar to strdup function, but returns string.
string string_strdup(struct allocator *a, char *data);
// Duplicates string.
string string_dup(struct allocator *a, string str);
// Evaluates to pointer to string end. Useful while parsing, where pointer
// arithmetic is used, since this value can be used for direct comparison,
// without fallback to length.
#define STRING_END(_str) ((_str).data + (_str).len)

#endif
