/*
Author: Holodome
Date: 21.10.2021
File: src/lib/text_file_lexer.h
Version: 0

Utility for parsing text files with not very compilcated syntax
*/
#ifndef TEXT_FILE_LEXER_H
#define TEXT_FILE_LEXER_H

#include "lib/general.h"

enum {
    TEXT_FILE_TOKEN_EOF   = 0x100,
    TEXT_FILE_TOKEN_IDENT = 0x101,
    TEXT_FILE_TOKEN_INT   = 0x102,
    TEXT_FILE_TOKEN_REAL  = 0x103,
    TEXT_FILE_TOKEN_STR   = 0x104,
};

typedef struct {
    u8 *buf;
    u8 *buf_at;
    u8 *buf_eof;
    
    u8 *string_buf;
    u32 string_buf_size;
    
    u32 line;
    u32 symb;
    
    bool has_errors;
    const char *error_string;
    
    u16 token_kind;
    i64 int_valie;
    f64 real_value;
} Text_Lexer;

Text_Lexer text_lexer(void *buf, uintptr_t buf_size, 
                      void *string_buf, u32 string_buf_size);
bool text_lexer_token(Text_Lexer *lexer);

#endif