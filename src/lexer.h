/*
Author: Holodome
Date: 10.10.2021
File: src/new_lexer.h
Version: 0
*/
#ifndef LEXER_H
#define LEXER_H
#include "lib/general.h"

#include "file_registry.h"  // File_ID
#include "string_storage.h" // String_ID

struct Out_Stream;
struct Compiler_Ctx;
struct Memory_Arena;
struct Lexer;

#define MAX_PREPROCESSOR_LINE_LENGTH 4096
#define MAX_PREPROCESSOR_MACROS      4096
#define MAX_NESTED_IFS               64
#define MAX_PP_MACRO_ARGS            128

enum {
    // End of stream. @NOTE(hl): Kinda not satisfied with the name, but the EOF doesn't make me happier either
    TOKEN_EOS        = 0x101,
    // Identifier 
    TOKEN_IDENT      = 0x102,
    // Special kind of identifiers
    TOKEN_KEYWORD    = 0x103,
    // String literal. Includes all type-specific ones
    TOKEN_STRING     = 0x104,
    // Number literal. Includes integer literals, floating-point literals and character constants.
    // Has type attached to it
    TOKEN_NUMBER     = 0x105,
    // Punctuator like , . + - += <<= etc.
    TOKEN_PUNCTUATOR = 0x106,
    // This could have been part of the string, but decided to stick this here for clarity
    TOKEN_PP_FILENAME          = 0x107,
    // Similar to strings, contains all that goes after #define AAA till the end of the string
    TOKEN_PP_DEFINE_DEFINITION = 0x108,
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
    PP_KEYWORD_DEFINE   = 0x1, // define
    PP_KEYWORD_UNDEF    = 0x2, // undef
    PP_KEYWORD_INCLUDE  = 0x3, // include
    PP_KEYWORD_IF       = 0x4, // if
    PP_KEYWORD_IFDEF    = 0x5, // ifdef
    PP_KEYWORD_IFNDEF   = 0x6, // ifndef
    PP_KEYWORD_ELSE     = 0x7, // else 
    PP_KEYWORD_ELIFDEF  = 0x8, // c2x elifdef
    PP_KEYWORD_ELIFNDEF = 0x9, // c2x elifndef
    PP_KEYWORD_PRAGMA   = 0xA, // pragma  
    PP_KEYWORD_ERROR    = 0xB, // error
    PP_KEYWORD_DEFINED  = 0xC, // defined
    PP_KEYWORD_LINE     = 0xD, // line
    PP_KEYWORD_ELIF     = 0xE, // elif
    PP_KEYWORD_ENDIF    = 0xF, // endif
    
    // PP_KEYWORD_VA_ARGS  = 0x10, // __VA_ARGS__
    
    PP_KEYWORD_SENTINEL,
};

enum {
    // @NOTE(hl): __func__, __PRETTY_FUNCTION__, __FUNCTION__ are not preprocessor macros
    PREDEFINED_MACRO_FILE = 0x1, // __FILE__
    PREDEFINED_MACRO_LINE = 0x2, // __LINE__
    PREDEFINED_MACRO_TIME = 0x3, // __TIME__
    PREDEFINED_MACRO_DATE = 0x4, // __DATE__ 
};

enum {
    PUNCTUATOR_IRSHIFT = 0x101, // >>=
    PUNCTUATOR_ILSHIFT = 0x102, // <<= 
    PUNCTUATOR_VARARGS = 0x103, // ...
    PUNCTUATOR_IADD    = 0x104, // +=
    PUNCTUATOR_ISUB    = 0x105, // -=
    PUNCTUATOR_IMUL    = 0x106, // *=
    PUNCTUATOR_IDIV    = 0x107, // /=
    PUNCTUATOR_IMOD    = 0x108, // %= 
    PUNCTUATOR_IAND    = 0x109, // &=
    PUNCTUATOR_IOR     = 0x10A, // |=
    PUNCTUATOR_IXOR    = 0x10B, // ^=
    PUNCTUATOR_INC     = 0x10C, // ++
    PUNCTUATOR_DEC     = 0x10D, // --
    PUNCTUATOR_RSHIFT  = 0x10E, // >>
    PUNCTUATOR_LSHIFT  = 0x10F, // <<
    PUNCTUATOR_LAND    = 0x110, // &&
    PUNCTUATOR_LOR     = 0x111, // ||
    PUNCTUATOR_EQ      = 0x112, // ==
    PUNCTUATOR_NEQ     = 0x113, // != 
    PUNCTUATOR_LEQ     = 0x114, // <= 
    PUNCTUATOR_GEQ     = 0x115, // >= 
    PUNCTUATOR_ARROW   = 0x116, // ->
    PUNCTUATOR_DOUBLE_HASH = 0x117, // ##
    
    PUNCTUATOR_SENTINEL,
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
    const Src_Loc *src_loc;
    union {
        struct {
            const char *str;
            // String_ID str;
        } str, ident, filename;
        u32 kw;
        u32 punct;
        struct {
            u32 type;
            const char *string;
            // @TODO(hl):
            // union {
            //     u64 uint_value;
            //     i64 sint_value;
            //     long double real_value;
            //     Float_Literal float_literal;
            // };
        } number;
    };
} Token;

#define IS_PUNCT(_tok, _punct) ((_tok)->kind == TOKEN_PUNCTUATOR && (_tok)->punct == (_punct))

u32 fmt_token_kind(struct Out_Stream *stream, u32 kind);
u32 fmt_token(struct Out_Stream *stream, Token *token);

typedef struct Token_Stack_Entry {
    Token *token;
    struct Token_Stack_Entry *next;
} Token_Stack_Entry;

enum {
    // Reading from file
    LEXER_BUFFER_FILE      = 0x1,
    // Macro expansion
    LEXER_BUFFER_MACRO     = 0x2,
    // Macro argument expansion
    LEXER_BUFFER_MACRO_ARG = 0x3,
    // Concatenated string expansion 
    LEXER_BUFFER_CONCAT    = 0x4, 
};

// Represents buffer from which data needs to be parsed as source.
typedef struct Lexer_Buffer {
    const char *buf;
    u32         size;
    
    const char *at;
    
    u8 kind;
    u32 line;
    u32 symb;
    union {
        File_ID file_id; 
        struct {
            struct PP_Macro *macro;
            // If resolving macro is function-like, save what strings
            // its parameters should be substitueted to
            const char *macro_args[MAX_PP_MACRO_ARGS];
            u32         macro_arg_count;
        };
    };
    struct Lexer_Buffer *next;
} Lexer_Buffer;

u8 lexbuf_peek(Lexer_Buffer *buffer);
u32 lexbuf_advance(Lexer_Buffer *buffer, u32 n);
bool lexbuf_parse(Lexer_Buffer *buffer, const char *lit);

typedef struct PP_Macro {
    u64 id;
    Src_Loc *loc;
    const char *name;
    bool is_function_like;
    bool has_varargs;
    u32  varargs_idx; // Idx where ... appeared
    const char *arg_names[MAX_PP_MACRO_ARGS];
    u32         arg_count;
    
    char definition[MAX_PREPROCESSOR_LINE_LENGTH];
    u32  definition_len;
} PP_Macro;

typedef struct {
    bool is_handled;
} PP_Nested_If;

typedef struct Lexer {
    struct Memory_Arena *arena;
    struct Compiler_Ctx *ctx;
    // Storage of buffers that are used to parse text from
    // This can include files, macros, macro arguments
    Lexer_Buffer *buffer_stack;
    Lexer_Buffer *buffer_freelist;
    // Current include depth. Need to keep track of it so we don't run into infinite loop
    u32 buffer_stack_size;
    // If some of the parent buffers is preprocessor - 
    // this is needed to make stringifying and joining strings possible
    // @NOTE(hl): This is number, but in code is used as a boolean,
    // could make union to make this more clear
    u32 is_in_preprocessor_ctx;  
    
    // @TODO(hl): Might want to move this to pointers
    PP_Macro     macros[MAX_PREPROCESSOR_MACROS];
    Hash_Table64 macro_hash;
    u32          next_macro_slot;
    // Needs to be stored so we know whether next #if should be checked or skipped 
    PP_Nested_If nested_ifs[MAX_NESTED_IFS];
    u32          nested_if_cursor;
    // Preprocessor    preprocessor;
    // @NOTE(hl): Stack structure used to temporarily store tokens when peekign ahead of current.
    // Because we can't really tell how much tokens we would want to peek, it is free-list based.
    u32                token_stack_size;
    Token_Stack_Entry *token_stack;
    Token_Stack_Entry *token_stack_freelist;
    
    char *scratch_buffer;
    u32 scratch_buf_size;
    u32 scratch_buf_capacity;
} Lexer;

Lexer *create_lexer(struct Compiler_Ctx *ctx, const char *filename);
Token *peek_tok(Lexer *lexer);
Token *peek_tok_forward(Lexer *lexer, u32 forward);
void eat_tok(Lexer *lexer);

void pp_push_nested_if(Lexer *lexer, bool is_handled);
bool pp_get_nested_if_handled(Lexer *lexer);
void pp_set_nested_if_handled(Lexer *lexer);
void pp_pop_nested_if(Lexer *lexer);

PP_Macro *pp_get(Lexer *lexer, const char *name);
PP_Macro *pp_define(Lexer *lexer, const char *name);
void pp_undef(Lexer *lexer, const char *name);

void add_buffer_to_stack(Lexer *lexer, Lexer_Buffer *entry);
void add_buffer_to_stack_file(Lexer *lexer, const char *filename);
void add_buffer_to_stack_macro_expansion(Lexer *lexer, PP_Macro *macro);
void add_buffer_to_stack_macro_arg_expansion(Lexer *lexer, PP_Macro *macro, Lexer_Buffer *lexbuf);
void add_buffer_to_stack_concat(Lexer *lexer);
void pop_buffer_from_stack(Lexer *lexer);
Lexer_Buffer *get_current_buf(Lexer *lexer);

void add_token_to_stack(Lexer *lexer, Token *token);
void pop_token_from_stack(Lexer *lexer);
Token *get_current_token(Lexer *lexer);

u8 peek_codepoint(Lexer *lexer);
void advance(Lexer *lexer, u32 n);
bool parse(Lexer *lexer, const char *lit);
bool skip_spaces(Lexer *lexer);

#endif
