#ifndef LEXER_H
#define LEXER_H

#include "types.h"

struct buffer_writer;

// Kind of token
typedef enum {
    // End of file marker. Always the last token
    PP_TOK_EOF = 0x0,
    // Identifier
    PP_TOK_ID = 0x1,
    // Number (float or int)
    PP_TOK_NUM = 0x2,
    // String (utf8)
    PP_TOK_STR = 0x3,
    // Punctuator
    PP_TOK_PUNCT = 0x4,
    // Typically non-utf8 characters encountered in invalid locations (ike
    // inside non-expanded #if)
    PP_TOK_OTHER = 0x5
} pp_token_kind;

// Type of string literal.
// Character literal are also considered strings.
// S... means string, C... means character.
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

// Kind of punctuator. Multisymbol puncutators start from 256, reserving
// lower enum values for ASCII ones. This enum may be completely the same as C
// one, but they made different to fully differentiate betweeen C and
// Preprocessor tokens. In reality only two punctuators that are allowed in
// preprocessor are not allowed in language: # and ##.
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
    PP_TOK_PUNCT_LSHIFT  = 0x10E,  // <<
    PP_TOK_PUNCT_LAND    = 0x10F,  // &&
    PP_TOK_PUNCT_LOR     = 0x110,  // ||
    PP_TOK_PUNCT_EQ      = 0x111,  // ==
    PP_TOK_PUNCT_NEQ     = 0x112,  // !=
    PP_TOK_PUNCT_LEQ     = 0x113,  // <=
    PP_TOK_PUNCT_GEQ     = 0x114,  // >=
    PP_TOK_PUNCT_DHASH   = 0x115,  // ##
} pp_punct_kind;

// Kind of tokens returned by pp_lexer
typedef struct pp_token {
    // Not used in pp_lexer directly, but in preprocessor
    struct pp_token *next;
    pp_token_kind kind;
    pp_string_kind str_kind;
    pp_punct_kind punct_kind;
    string str;
    // This information must be present to parse preprocessor directives.
    // Function-like macros are sensetive to spaces and all preprocessor
    // directives take one full source line.
    // This can also be used to make somewhat-readable printing of tokens.
    bool has_whitespace;
    bool at_line_start;

    source_loc loc;
#if HOLOC_DEBUG
    char *_debug_info;
#endif
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
// correct c tokens. For converting see c_lang and preprocessor.
typedef struct pp_lexer {
    // Start of data that we parse
    char *data;
    // End of data, so range checking is always done in pointers and not with
    // start-size
    char *eof;
    // Current byte
    char *cursor;
    // Where last \n was encountered. This can be used to generate column number
    char *last_line_start;
    // Byte start of token. This with last_line_start can be used to generate
    // column number
    char *tok_start;
    // Line number
    uint32_t line;
    // External buffer used for writing token strings to it
    char *tok_buf;
    // Size of tok_buf
    uint32_t tok_buf_capacity;
    // Occupied size if tok_buf
    uint32_t tok_buf_len;
} pp_lexer;

void pp_tok_init_eof(pp_token *tok);
void pp_tok_init_num(pp_token *tok, char *num_str, struct allocator *a);

// Initializes all members of lex to parse given data.
void init_pp_lexer(pp_lexer *lex, char *data, char *eof, char *tok_buf,
                   uint32_t tok_buf_size);

// Generates one new token at writes it in lexer->tok
bool pp_lexer_parse(pp_lexer *lexer, pp_token *tok);
// Formats token like it is seen in code
void fmt_pp_tokw(struct buffer_writer *w, pp_token *tok);
uint32_t fmt_pp_tok(char *buf, uint32_t buf_len, pp_token *tok);
// Formats token like it is seen in code, while also providing token kind
// information
void fmt_pp_tok_verbosew(struct buffer_writer *w, pp_token *tok);
uint32_t fmt_pp_tok_verbose(char *buf, uint32_t buf_len, pp_token *tok);
#define PP_TOK_IS_PUNCT(_tok, _punct) \
    (((_tok)->kind == PP_TOK_PUNCT) && ((_tok)->punct_kind == (_punct)))

#endif
