#pragma once 

#include "general.h"
#include "memory.h"

#include "filesystem.h" // SourceLocation

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
    SourceLocation source_loc;
} Token;

// Structure storing state of parsing some buffer 
//
// Different parts of program are designed to be used as 'modules'. Thus, they should have minimal ways to communcate with each other.
// For Tokenizer it was decided to not give it access to filesystem, because in ceratain cases (like when we simply want to parse string)
// invloving separate system would invlove too much friction. Instead, tokenizer is provided with a string defining name of the source
// 
// Tokenizer does not do error handling in inselft, instead it provides error tokens (TOKEN_NONE) that usage code can decide what to do with.
// This way error handling can be centralized, because errors can happen in several places, not only in tokenizing.
typedef struct Tokenizer {
    MemoryArena arena;
    // Buffer is copied on tokenizer arena and deleted with tokenizer
    // Tag string set by usage code. It is set into source_location in tokens and can be used elsewhere
    char *source_name;
    u8 *buffer;
    uptr buffer_size;
    
    u8 *cursor;
    // Current symbol
    u32 symb;
    u32 line_number;
    u32 symb_number;

    Token *active_token;
} Tokenizer;

Tokenizer create_tokenizer(const void *buffer, uptr buffer_size, 
    const char *name);
// Deletes all tokens
void delete_tokenizer(Tokenizer *tokenizer);
// Returns current token. Stores token until it's eaten
Token *peek_tok(Tokenizer *tokenizer);
// Tell tokenizer to return next token on next peek_tok call
void eat_tok(Tokenizer *tokenizer);
// call eat_tok and return peek_tok
Token *peek_next_tok(Tokenizer *tokenizer);

void fmt_tok(char *buf, uptr buf_sz, Token *token);