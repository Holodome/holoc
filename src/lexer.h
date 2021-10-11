/*
Author: Holodome
Date: 10.10.2021
File: src/new_lexer.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "lib/stream.h"     // In_UTF8_Stream

#include "file_registry.h"  // File_ID
#include "string_storage.h" // String_ID

struct In_UTF8_Stream;
struct Compiler_Ctx;
struct Memory_Arena;
struct Lexer;

enum {
    TOKEN_EOS      = 0x101,
    TOKEN_IDENT    = 0x102,
    TOKEN_KEYWORD  = 0x103,
    TOKEN_STRING   = 0x104,
    TOKEN_NUMBER   = 0x105,
    TOKEN_OPERATOR = 0x106
};

enum {
    KEYWORD_AUTO          = 0x1, // auto
    KEYWORD_BREAK         = 0x2, // break
    KEYWORD_CASE          = 0x3, // case
    KEYWORD_CHAR          = 0x4, // char
    KEYWORD_CONST         = 0x5, // const
    KEYWORD_CONTINUE      = 0x6, // continue
    KEYWORD_DEFAULT       = 0x7, // default
    KEYWORD_DO            = 0x8, // do
    KEYWORD_DOUBLE        = 0x9, // double
    KEYWORD_ELSE          = 0xA, // else 
    KEYWORD_ENUM          = 0xB, // enum 
    KEYWORD_EXTERN        = 0xC, // extern
    KEYWORD_FLOAT         = 0xD, // float
    KEYWORD_FOR           = 0xE, // for 
    KEYWORD_GOTO          = 0xF, // goto
    KEYWORD_IF            = 0x10, // if
    KEYWORD_INLINE        = 0x11, // inline
    KEYWORD_INT           = 0x12, // int
    KEYWORD_LONG          = 0x13, // long
    KEYWORD_REGISTER      = 0x14, // register
    KEYWORD_RESTRICT      = 0x15, // restrict
    KEYWORD_RETURN        = 0x16, // return
    KEYWORD_SHORT         = 0x17, // short
    KEYWORD_SIGNED        = 0x18, // signed
    KEYWORD_SIZEOF        = 0x19, // unsigned 
    KEYWORD_STATIC        = 0x1A, // static 
    KEYWORD_STRUCT        = 0x1B, // struct
    KEYWORD_SWITCH        = 0x1C, // switch 
    KEYWORD_TYPEDEF       = 0x1D, // typedef
    KEYWORD_UNION         = 0x1E, // union 
    KEYWORD_UNSIGNED      = 0x1F, // unsigned
    KEYWORD_VOID          = 0x20, // void
    KEYWORD_VOLATILE      = 0x21, // volatile
    KEYWORD_WHILE         = 0x22, // while
    KEYWORD_ALIGNAS       = 0x23, // _Alignas
    KEYWORD_ALIGNOF       = 0x24, // _Alignof
    KEYWORD_ATOMIC        = 0x25, // _Atomic
    KEYWORD_BOOL          = 0x26, // _Bool
    KEYWORD_COMPLEX       = 0x27, // _Complex
    KEYWORD_DECIMAL128    = 0x28, // c2x _Decimal128 
    KEYWORD_DECIMAL32     = 0x29, // c2x _Decimal32
    KEYWORD_DECIMAL64     = 0x2A, // c2x _Decimal64
    KEYWORD_GENERIC       = 0x2B, // _Generic
    KEYWORD_IMAGINARY     = 0x2C, // _Imaginary
    KEYWORD_NORETURN      = 0x2D, // _Noreturn
    KEYWORD_STATIC_ASSERT = 0x2E, // _Static_assert
    KEYWORD_THREAD_LOCAL  = 0x2F, // _Thread_local
    KEYWORD_PRAGMA        = 0x30, // _Pragma
    
    KEYWORD_SENTINEL,
};

enum {
    PP_KEYWORD_DEFINE   = 0x101, // define
    PP_KEYWORD_UNDEF    = 0x102, // undef
    PP_KEYWORD_INCLUDE  = 0x103, // include
    PP_KEYWORD_IF       = 0x104, // if
    PP_KEYWORD_IFDEF    = 0x105, // ifdef
    PP_KEYWORD_IFNDEF   = 0x106, // ifndef
    PP_KEYWORD_ELSE     = 0x107, // else 
    PP_KEYWORD_ELIFDEF  = 0x108, // c2x elifdef
    PP_KEYWORD_ELIFNDEF = 0x109, // c2x elifndef
    PP_KEYWORD_PRAGMA   = 0x10A, // pragma  
};

enum {
    OPERATOR_IRSHIFT = 0x101, // >>=
    OPERATOR_ILSHIFT = 0x102, // <<= 
    OPERATOR_IADD    = 0x103, // +=
    OPERATOR_ISUB    = 0x104, // -=
    OPERATOR_IMUL    = 0x105, // *=
    OPERATOR_IDIV    = 0x106, // /=
    OPERATOR_IMOD    = 0x107, // %= 
    OPERATOR_IAND    = 0x108, // &=
    OPERATOR_IOR     = 0x109, // |=
    OPERATOR_IXOR    = 0x10A, // ^=
    OPERATOR_INC     = 0x10B, // ++
    OPERATOR_DEC     = 0x10C, // --
    OPERATOR_RSHIFT  = 0x10D, // >>
    OPERATOR_LSHIFT  = 0x10E, // <<
    OPERATOR_LAND    = 0x10F, // &&
    OPERATOR_LOR     = 0x110, // ||
    OPERATOR_EQ      = 0x111, // ==
    OPERATOR_NEQ     = 0x112, // != 
    OPERATOR_LEQ     = 0x113, // <= 
    OPERATOR_GEQ     = 0x114, // >= 
    OPERATOR_ARROW   = 0x115, // ->
    
    OPERATOR_SENTINEL,
};

enum {
    C_TYPE_VOID   = 0x0, // void
    
    C_TYPE_CHAR   = 0x1, // char 
    C_TYPE_SCHAR  = 0x2, // signed char 
    C_TYPE_UCHAR  = 0x3, // unsigned char 
    C_TYPE_WCHAR  = 0x4, // wchar_t
    C_TYPE_CHAR16 = 0x5, // char16_t
    C_TYPE_CHAR32 = 0x6, // char32_t
    
    C_TYPE_SINT   = 0x7, // signed int
    C_TYPE_UINT   = 0x8, // unsigned int
    C_TYPE_SLINT  = 0x9, // signed long int
    C_TYPE_ULINT  = 0xA, // unsigned long int
    C_TYPE_SLLINT = 0xB, // signed long long int
    C_TYPE_ULLINT = 0xC, // unsigned long long int
    C_TYPE_SSINT  = 0xD, // signed short int
    C_TYPE_USINT  = 0xE, // unsigned short int
    
    C_TYPE_FLOAT   = 0xF,  // float
    C_TYPE_DOUBLE  = 0x10, // double
    C_TYPE_LDOUBLE = 0x11, // long double
    
    C_TYPE_DECIMAL32  = 0x12, // _Decimal32
    C_TYPE_DECIMAL64  = 0x13, // _Decimal64
    C_TYPE_DECIMAL128 = 0x14, // _Decimal128
  
    C_TYPE_BOOL = 0x15, // _Bool
};

typedef struct {
    u64 whole_number;
    u64 fraction;
    u64 fraction_part;
    u8  exponent_sign;
    u32 exponent;
} Float_Literal;

typedef struct Token {
    u32 kind;
    Src_Loc src_loc;
    union {
        struct {
            String_ID str;
        } str;
        struct {
            u32 kind;
        } kw;
        struct {
            u32 kind;
        } operator; 
        struct {
            u32 type;
            union {
                u64 int_value;
                Float_Literal float_literal;
            };
        } number;
    };
} Token;

typedef struct Token_Stack_Entry {
    Token *token;
    struct Token_Stack_Entry *next;
} Token_Stack_Entry;

enum {
    PP_TOKEN_NONE     = 0x0, // correspond to regular token
    PP_TOKEN_FILENAME = 0x1, // separate type because preprocessor allows <> to mean same as ""
};

typedef struct {
    u32 kind;
    union {
        Token *token;
        struct {
            String_ID filename;
            bool is_global_search;
        } filename;
    };
} Preprocessor_Token;

#define MAXIMUM_MACRO_IDENTIFIER_COUNT 4096

typedef struct Preprocessor_Macro {
    String_ID ident;
} Preprocessor_Macro;

typedef struct {
    struct Memory_Arena *arena;
    
    Preprocessor_Macro macros[MAXIMUM_MACRO_IDENTIFIER_COUNT];
    Hash_Table64       macro_hash_table;
} Preprocessor;

void init_preprocessor(Preprocessor *preprocessor);
void preprocessor_parse_directive(struct Lexer *lexer);

typedef struct Lexer {
    struct Memory_Arena *arena;
    struct Compiler_Ctx *ctx;
    
    u32 *unicode_buf;
    u32  unicode_buf_at;
    u32  unicode_buf_capacity;
    In_UTF8_Stream *stream;
    Src_Loc         curr_loc;
    Preprocessor    preprocessor;
    // @NOTE(hl): Stack structure used to temporarily store tokens when peekign ahead of current.
    // Because we can't really tell how much tokens we would want to peek, it is free-list based.
    u32                token_stack_size;
    Token_Stack_Entry *token_stack;
    Token_Stack_Entry *token_stack_freelist;
} Lexer;

Lexer *create_lexer(struct Compiler_Ctx *ctx, struct In_UTF8_Stream *stream);
Token *peek_tok(Lexer *lexer);
void eat_tok(Lexer *lexer);