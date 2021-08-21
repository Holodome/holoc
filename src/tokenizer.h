#pragma once 

#include "general.h"
#include "memory.h"

enum {
    TOKEN_NONE = 0x0,
    // Mentally insert ASCII tokens here...
    TOKEN_EOS = 0x100, 
    TOKEN_IDENT, // value_str
    TOKEN_INT, // value_int
    TOKEN_REAL, // value_real
    TOKEN_STR, // value_str
};

typedef struct Token {
    u32 kind;
    union {
        const char *value_str;
        i64 value_int;
        f64 value_real;
    };
} Token;

typedef struct Tokenizer {
    MemoryArena arena;
    // Buffer is copied on tokenizer arena and deleted with tokenizer
    u8 *buffer;
    uptr buffer_size;
    
    u8 *cursor;
    // Current symbol
    u32 symb;
    u32 line_number;
    u32 symb_number;

    Token *active_token;
} Tokenizer;

Tokenizer create_tokenizer(const void *buffer, uptr buffer_size);
// Deletes all tokens
void delete_tokenizer(Tokenizer *tokenizer);
// Returns current token. Stores token until it's eaten
Token *peek_tok(Tokenizer *tokenizer);
// Tell tokenizer to return next token on next peek_tok call
void eat_tok(Tokenizer *tokenizer);
