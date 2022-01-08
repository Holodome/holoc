/*
Author: Holodome
Date: 18.11.2021
File: src/string.h
Version: 0
*/
#ifndef STRING_H
#define STRING_H

#include "types.h"

typedef struct {
    bool is_found;
    uint32_t idx;
} string_find_result;

// Wrapper for string creation call
#define string(_data, _len) string_(_data, _len)
string string_(char *_data, uint32_t len);
// Construct string from string literal
#define WRAP_Z(_z)       string(_z, sizeof(_z) - 1)
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

bool is_ident_start(uint32_t symb);
bool is_ident(uint32_t symb);

#endif
