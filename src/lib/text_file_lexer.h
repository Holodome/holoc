/*
Author: Holodome
Date: 21.10.2021
File: src/lib/text_file_lexer.h
Version: 1

Utility for parsing utf8 text files with not very compilcated syntax
It supports parsing of identifiers, inegers, float and strings

This works without memory allocations, using only buffer for writing strings to.
This can be easy-to-drop soluation for parsing config/other simple formats.
*/
#ifndef TEXT_LEXER_H
#define TEXT_LEXER_H

#include "lib/general.h"

enum {
    TEXT_LEXER_TOKEN_EOF   = 0x100,
    TEXT_LEXER_TOKEN_IDENT = 0x101,
    TEXT_LEXER_TOKEN_INT   = 0x102,
    TEXT_LEXER_TOKEN_REAL  = 0x103,
    TEXT_LEXER_TOKEN_STR   = 0x104,
};

typedef struct {
    u8 *buf;
    u8 *buf_at;
    u8 *buf_eof;
    
    union {
        u8   *string_buf_u8;
        char *string_buf;
    };
    u32 string_buf_used;
    u32 string_buf_size;
    
    u32 line;
    u32 symb;
    
    u16 token_kind;
    i64 int_valie;
    f64 real_value;
    
    bool should_make_new;
} Text_Lexer;

Text_Lexer text_lexer(void *buf, uintptr_t buf_size, 
                      void *string_buf, u32 string_buf_size);
bool text_lexer_token(Text_Lexer *lexer);
void text_lexer_eat(Text_Lexer *lexer);
bool text_lexer_next(Text_Lexer *lexer);

#endif