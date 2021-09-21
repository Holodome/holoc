#pragma once 

#include "general.h"
#include "memory.h"

#include "stream.h" // InStream
#include "filesystem.h" // SourceLocation

#define TOKENIZER_DEFAULT_SCRATCH_BUFFER_SIZE KB(1)
#define TOKEN_GENERAL 0x100
#define TOKEN_KEYWORD 0x120
#define TOKEN_MULTISYMB 0x160
#define IS_TOKEN_ASCII(_tok) (0 <= (_tok) && (_tok) <= 255)
#define IS_TOKEN_GENERAL(_tok) (TOKEN_GENERAL <= (_tok) && (_tok) < TOKEN_KEYWORD)
#define IS_TOKEN_KEYWORD(_tok) (TOKEN_KEYWORD <= (_tok) && (_tok) < TOKEN_MULTISYMB)
#define IS_TOKEN_MULTISYMB(_tok) (TOKEN_MULTISYMB <= (_tok) && (_tok) < TOKEN_COUNT)
// Token kind space is reserved to handle expanding easilly
// Values 0-255 correspond to single-symbol ASCII tokens
// Values 256-287 (32) correspond to basic kinds
// Values 288-351 (64) correspond to keywords 
// Values 352-(TOKEN_COUNT - 1) correspond to multisymbol tokens
enum {
    TOKEN_NONE = 0x0,
    // Mentally insert ASCII tokens here...
    TOKEN_EOS = TOKEN_GENERAL,
    TOKEN_ERROR, 
    TOKEN_IDENT, // value_str
    TOKEN_INT, // value_int
    TOKEN_REAL, // value_real
    TOKEN_STR, // value_str,
    
    TOKEN_KW_PRINT = TOKEN_KEYWORD, // print
    TOKEN_KW_WHILE, // while
    TOKEN_KW_RETURN, // return 
    TOKEN_KW_IF, // if 
    TOKEN_KW_ELSE, // else
    TOKEN_KW__END,
    
    // Digraphs and trigraphs
    TOKEN_ILSHIFT = TOKEN_MULTISYMB, // <<=
    TOKEN_IRSHIFT, // >>= 
    
    TOKEN_CONSTANT, // ::
    TOKEN_AUTO_DECL, // :=
    TOKEN_ARROW, // ->
    TOKEN_LE, // <=
    TOKEN_GE, // >=
    TOKEN_EQ, // == 
    TOKEN_NEQ, // !=
    TOKEN_LSHIFT, // <<
    TOKEN_RSHIFT, // >>
    TOKEN_IADD, // +=
    TOKEN_ISUB, // -=
    TOKEN_IAND, // &=
    TOKEN_IOR, // |=
    TOKEN_IXOR, // ^=
    TOKEN_IDIV, // /=
    TOKEN_IMUL, // *=
    TOKEN_IMOD, // %=
    TOKEN_LOGICAL_AND, // &&
    TOKEN_LOGICAL_OR, // ||
    
    TOKEN_COUNT,
};

b32 is_token_assign(u32 tok);

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
    InStream *st;
    
    u32 line_number;
    u32 symb_number;
    // Buffer for internal use. When parsing multiline symbols, 
    // this buffer is used for it. Basically size of scratch buffer defines maximum length 
    // of identifier.
    // @TODO There is special case for big strings - we need to think how we would handle that
    u32 scratch_buffer_size;
    u8 *scratch_buffer;
    
    Token *active_token;
} Tokenizer;

// @NOTE Tokenizer is an object that has its own big lifetime, 
// and so we return pointer here and allocate all memory using arena located inside
// the tokenizer, making it kinda OOPy API
Tokenizer *create_tokenizer(InStream *st);
// Deletes all tokens
void destroy_tokenizer(Tokenizer *tokenizer);
// Returns current token. Stores token until it's eaten
Token *peek_tok(Tokenizer *tokenizer);
// Tell tokenizer to return next token on next peek_tok call
void eat_tok(Tokenizer *tokenizer);
// call eat_tok and return peek_tok
Token *peek_next_tok(Tokenizer *tokenizer);

uptr fmt_tok_kind(char *buf, uptr buf_sz, u32 kind);
uptr fmt_tok(char *buf, uptr buf_sz, Token *token);