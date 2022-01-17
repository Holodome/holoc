#ifndef LEXER_H
#define LEXER_H

#include "types.h"

typedef enum {
    PP_TOK_EOF   = 0x0,
    PP_TOK_ID    = 0x1,
    PP_TOK_NUM   = 0x2,
    PP_TOK_STR   = 0x3,
    PP_TOK_PUNCT = 0x4,
    PP_TOK_OTHER = 0x5
} pp_token_kind;

typedef enum {
    PP_TOK_STR_NONE   = 0x0,
    PP_TOK_STR_SCHAR  = 0x1,   // ""
    PP_TOK_STR_SUTF8  = 0x2,   // u8""
    PP_TOK_STR_SUTF16 = 0x3,   // u""
    PP_TOK_STR_SUTF32 = 0x4,   // U""
    PP_TOK_STR_SWIDE  = 0x5,   // L""
    PP_TOK_STR_CCHAR  = 0x11,  // ''
    PP_TOK_STR_CUTF8  = 0x12,  // u8''
    PP_TOK_STR_CUTF16 = 0x13,  // u''
    PP_TOK_STR_CUTF32 = 0x14,  // U''
    PP_TOK_STR_CWIDE  = 0x15,  // L''
} pp_string_kind;

typedef enum {
    PP_TOK_PUNCT_IRSHIFT = 0x100,  // >>=
    PP_TOK_PUNCT_ILSHIFT = 0x101,  // <<=
    PP_TOK_PUNCT_VARARGS = 0x102,  // ...
    PP_TOK_PUNCT_IADD    = 0x103,  // +=
    PP_TOK_PUNCT_ISUB    = 0x104,  // -=
    PP_TOK_PUNCT_IMUL    = 0x105,  // *=
    PP_TOK_PUNCT_IDIV    = 0x106,  // /=
    PP_TOK_PUNCT_IMOD    = 0x107,  // %=
    PP_TOK_PUNCT_IAND    = 0x108,  // &=
    PP_TOK_PUNCT_IOR     = 0x109,  // |=
    PP_TOK_PUNCT_IXOR    = 0x10A,  // ^=
    PP_TOK_PUNCT_INC     = 0x10B,  // ++
    PP_TOK_PUNCT_DEC     = 0x10C,  // --
    PP_TOK_PUNCT_RSHIFT  = 0x10D,  // >>
    PP_TOK_PUNCT_LSHIFT  = 0x10F,  // <<
    PP_TOK_PUNCT_LAND    = 0x110,  // &&
    PP_TOK_PUNCT_LOR     = 0x111,  // ||
    PP_TOK_PUNCT_EQ      = 0x112,  // ==
    PP_TOK_PUNCT_NEQ     = 0x113,  // !=
    PP_TOK_PUNCT_LEQ     = 0x114,  // <=
    PP_TOK_PUNCT_GEQ     = 0x115,  // >=
    PP_TOK_PUNCT_DHASH   = 0x116,  // ##
} pp_punct_kind;

// Kind of tokens returned by pp_lexer
typedef struct pp_token {
    uint32_t kind;
    pp_string_kind str_kind;
    pp_punct_kind punct_kind;
    string str;
    // TODO: Replace this with flags
    // TODO: Replace this wiht number of whitespaces, so we can fully
    // reconstruct source tree
    bool has_whitespace;
    // TODO: Replace this with skipped line number, so we can fully reconstruct
    // source tree
    bool at_line_start;
    // Not used in pp_lexer directly, but in preprocessor
    struct pp_token *next;
} pp_token;

// Structure holding state needed to process the source file at the first stage
// Tokens returned by pp_lexer are quite similar, but different from tokens used
// in actual source tree parsing. pp_lexer is designed to be a simple drop-in
// solution for generating tokens, not doing any memory allocations and thus
// being flexible to use in further file processing stage - preprocessing
//
// Due to the nature of the c language, strings of all tokens produced at
// preprocessing stage may or may not correspond to actual tokens. This is
// actual for numbers. However, all tokens that are not produced from
// preprocessing context in source (not part of macro invocation) _shold_ be
// correct c tokens.
typedef struct pp_lexer {
    char *data;
    char *eof;
    char *cursor;

    pp_token tok;
    char *tok_buf;
    uint32_t tok_buf_capacity;
    uint32_t tok_buf_len;
} pp_lexer;

// Generates one new token at writes it in lexer->tok
bool pp_lexer_parse(pp_lexer *lexer);
// Formats token like it is seen in code
uint32_t fmt_pp_tok(pp_token *tok, char *buf, uint32_t buf_len);
// Formats token like it is seen in code, while also providing token kind
// information
uint32_t fmt_pp_tok_verbose(pp_token *tok, char *buf, uint32_t buf_len);

#endif
