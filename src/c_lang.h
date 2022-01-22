#ifndef C_LANG_H
#define C_LANG_H

#include "types.h"

struct pp_token;
struct allocator;
struct buffer_writer;

typedef enum {
    C_KW_AUTO          = 0x1,   // auto
    C_KW_BREAK         = 0x2,   // break
    C_KW_CASE          = 0x3,   // case
    C_KW_CHAR          = 0x4,   // char
    C_KW_CONST         = 0x5,   // const
    C_KW_CONTINUE      = 0x6,   // continue
    C_KW_DEFAULT       = 0x7,   // default
    C_KW_DO            = 0x8,   // do
    C_KW_DOUBLE        = 0x9,   // double
    C_KW_ELSE          = 0xA,   // else
    C_KW_ENUM          = 0xB,   // enum
    C_KW_EXTERN        = 0xC,   // extern
    C_KW_FLOAT         = 0xD,   // float
    C_KW_FOR           = 0xE,   // for
    C_KW_GOTO          = 0xF,   // goto
    C_KW_IF            = 0x10,  // if
    C_KW_INLINE        = 0x11,  // inline
    C_KW_INT           = 0x12,  // int
    C_KW_LONG          = 0x13,  // long
    C_KW_REGISTER      = 0x14,  // register
    C_KW_RESTRICT      = 0x15,  // restrict
    C_KW_RETURN        = 0x16,  // return
    C_KW_SHORT         = 0x17,  // short
    C_KW_SIGNED        = 0x18,  // signed
    C_KW_SIZEOF        = 0x19,  // unsigned
    C_KW_STATIC        = 0x1A,  // static
    C_KW_STRUCT        = 0x1B,  // struct
    C_KW_SWITCH        = 0x1C,  // switch
    C_KW_TYPEDEF       = 0x1D,  // typedef
    C_KW_UNION         = 0x1E,  // union
    C_KW_UNSIGNED      = 0x1F,  // unsigned
    C_KW_VOID          = 0x20,  // void
    C_KW_VOLATILE      = 0x21,  // volatile
    C_KW_WHILE         = 0x22,  // while
    C_KW_ALIGNAS       = 0x23,  // _Alignas
    C_KW_ALIGNOF       = 0x24,  // _Alignof
    C_KW_ATOMIC        = 0x25,  // _Atomic
    C_KW_BOOL          = 0x26,  // _Bool
    C_KW_COMPLEX       = 0x27,  // _Complex
    C_KW_DECIMAL128    = 0x28,  // c2x _Decimal128
    C_KW_DECIMAL32     = 0x29,  // c2x _Decimal32
    C_KW_DECIMAL64     = 0x2A,  // c2x _Decimal64
    C_KW_GENERIC       = 0x2B,  // _Generic
    C_KW_IMAGINARY     = 0x2C,  // _Imaginary
    C_KW_NORETURN      = 0x2D,  // _Noreturn
    C_KW_STATIC_ASSERT = 0x2E,  // _Static_assert
    C_KW_THREAD_LOCAL  = 0x2F,  // _Thread_local
    C_KW_PRAGMA        = 0x30,  // _Pragma
} c_keyword_kind;

typedef enum {
    C_PUNCT_IRSHIFT = 0x101,  // >>=
    C_PUNCT_ILSHIFT = 0x102,  // <<=
    C_PUNCT_IADD    = 0x103,  // +=
    C_PUNCT_ISUB    = 0x104,  // -=
    C_PUNCT_IMUL    = 0x105,  // *=
    C_PUNCT_IDIV    = 0x106,  // /=
    C_PUNCT_IMOD    = 0x107,  // %=
    C_PUNCT_IAND    = 0x108,  // &=
    C_PUNCT_IOR     = 0x109,  // |=
    C_PUNCT_IXOR    = 0x10A,  // ^=
    C_PUNCT_INC     = 0x10B,  // ++
    C_PUNCT_DEC     = 0x10C,  // --
    C_PUNCT_RSHIFT  = 0x10D,  // >>
    C_PUNCT_LSHIFT  = 0x10E,  // <<
    C_PUNCT_LAND    = 0x11F,  // &&
    C_PUNCT_LOR     = 0x110,  // ||
    C_PUNCT_EQ      = 0x111,  // ==
    C_PUNCT_NEQ     = 0x112,  // !=
    C_PUNCT_LEQ     = 0x113,  // <=
    C_PUNCT_GEQ     = 0x114,  // >=
    C_PUNCT_VARARGS = 0x115,  // ... (in functions)
} c_punct_kind;

typedef enum token_kind {
    TOK_EOF   = 0x0,  // End of file
    TOK_ID    = 0x1,  // Identfier
    TOK_PUNCT = 0x2,  // Punctuator
    TOK_STR   = 0x3,  // String or arbitrary type
    TOK_NUM   = 0x4,  // Number (can be numeric literal or char literal)
    TOK_KW    = 0x5,  // Keyword
} token_kind;

typedef struct {
    string filename;
    uint32_t line;
    uint32_t column;
} source_location;

typedef struct token {
    struct token *next;
    source_location loc;

    token_kind kind;
    string str;
    uint64_t uint_value;
    long double float_value;
    struct c_type *type;
    c_punct_kind punct;
    c_keyword_kind kw;
#if HOLOC_DEBUG
    char *_debug_info;
#endif
} token;

typedef struct {
    uint64_t uint_value;
    long double float_value;
    struct c_type *type;
} fmt_c_num_args;

typedef struct {
    struct c_type *type;
    string str;
} fmt_c_str_args;

void fmt_c_numw(fmt_c_num_args args, struct buffer_writer *w);
uint32_t fmt_c_num(fmt_c_num_args args, char *buf, uint32_t buf_size);
void fmt_c_strw(fmt_c_str_args args, struct buffer_writer *w);
uint32_t fmt_c_str(fmt_c_str_args args, char *buf, uint32_t buf_size);
uint32_t fmt_token(token *tok, char *buf, uint32_t buf_size);
uint32_t fmt_token_verbose(token *tok, char *buf, uint32_t buf_size);
bool convert_pp_token(struct pp_token *pp_tok, token *tok, struct allocator *a);
#define IS_KW(_tok, _kw) ((_tok)->kind == TOK_KW && (_tok)->kw == (_kw))
#define IS_PUNCT(_tok, _punct) \
    ((_tok)->kind == TOK_PUNCT && (_tok)->punct == (_punct))

#endif
