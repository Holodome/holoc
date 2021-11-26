/*
Author: Holodome
Date: 21.11.2021
File: src/lexer.h
Version: 0
*/
#ifndef LEXER_H
#define LEXER_H

#include "types.h"
#include "c_types.h"

struct file_registry;

typedef struct {
    uint32_t value;
} c_lexbuf_id;

typedef struct {
    c_lexbuf_id buf;
    uint32_t line;
    uint32_t symb;
} c_lexer_loc;

// Enumeration of possible kinds of [[c_token]]
typedef enum {
    // End of file.
    // Used as sentinel to signal the end of input
    C_TOKEN_EOF   = 0x0,
    // String.
    // Represented as array of certain type
    C_TOKEN_STR   = 0x1,
    // Number.
    // Represented as value of specific c type, with a type marker
    C_TOKEN_NUM   = 0x2,
    // Identifier.
    // Represented as string
    C_TOKEN_IDENT = 0x3,
    // Punctuator.
    // Reprersented as enumeration [[c_punctuator]]
    C_TOKEN_PUNCT = 0x4,
    // Keyword
    // Represented as enumeration [[c_keyword]]
    C_TOKEN_KW    = 0x5
} c_token_kind;

// Enumeration of possible kinds of [[c_token_kind.C_TOKEN_KW]]
typedef enum {
    C_KEYWORD_AUTO          = 0x1, // auto
    C_KEYWORD_BREAK         = 0x2, // break
    C_KEYWORD_CASE          = 0x3, // case
    C_KEYWORD_CHAR          = 0x4, // char
    C_KEYWORD_CONST         = 0x5, // const
    C_KEYWORD_CONTINUE      = 0x6, // continue
    C_KEYWORD_DEFAULT       = 0x7, // default
    C_KEYWORD_DO            = 0x8, // do
    C_KEYWORD_DOUBLE        = 0x9, // double
    C_KEYWORD_ELSE          = 0xA, // else 
    C_KEYWORD_ENUM          = 0xB, // enum 
    C_KEYWORD_EXTERN        = 0xC, // extern
    C_KEYWORD_FLOAT         = 0xD, // float
    C_KEYWORD_FOR           = 0xE, // for 
    C_KEYWORD_GOTO          = 0xF, // goto
    C_KEYWORD_IF            = 0x10, // if
    C_KEYWORD_INLINE        = 0x11, // inline
    C_KEYWORD_INT           = 0x12, // int
    C_KEYWORD_LONG          = 0x13, // long
    C_KEYWORD_REGISTER      = 0x14, // register
    C_KEYWORD_RESTRICT      = 0x15, // restrict
    C_KEYWORD_RETURN        = 0x16, // return
    C_KEYWORD_SHORT         = 0x17, // short
    C_KEYWORD_SIGNED        = 0x18, // signed
    C_KEYWORD_SIZEOF        = 0x19, // unsigned 
    C_KEYWORD_STATIC        = 0x1A, // static 
    C_KEYWORD_STRUCT        = 0x1B, // struct
    C_KEYWORD_SWITCH        = 0x1C, // switch 
    C_KEYWORD_TYPEDEF       = 0x1D, // typedef
    C_KEYWORD_UNION         = 0x1E, // union 
    C_KEYWORD_UNSIGNED      = 0x1F, // unsigned
    C_KEYWORD_VOID          = 0x20, // void
    C_KEYWORD_VOLATILE      = 0x21, // volatile
    C_KEYWORD_WHILE         = 0x22, // while
    C_KEYWORD_ALIGNAS       = 0x23, // _Alignas
    C_KEYWORD_ALIGNOF       = 0x24, // _Alignof
    C_KEYWORD_ATOMIC        = 0x25, // _Atomic
    C_KEYWORD_BOOL          = 0x26, // _Bool
    C_KEYWORD_COMPLEX       = 0x27, // _Complex
    C_KEYWORD_DECIMAL128    = 0x28, // c2x _Decimal128 
    C_KEYWORD_DECIMAL32     = 0x29, // c2x _Decimal32
    C_KEYWORD_DECIMAL64     = 0x2A, // c2x _Decimal64
    C_KEYWORD_GENERIC       = 0x2B, // _Generic
    C_KEYWORD_IMAGINARY     = 0x2C, // _Imaginary
    C_KEYWORD_NORETURN      = 0x2D, // _Noreturn
    C_KEYWORD_STATIC_ASSERT = 0x2E, // _Static_assert
    C_KEYWORD_THREAD_LOCAL  = 0x2F, // _Thread_local
    C_KEYWORD_PRAGMA        = 0x30, // _Pragma
    
    C_KEYWORD_SENTINEL,
} c_keyword;

// Enumeration of possible kinds of [[c_token.C_TOKEN_PUNCT]]
typedef enum {
    C_PUNCT_IRSHIFT = 0x101, // >>=
    C_PUNCT_ILSHIFT = 0x102, // <<= 
    C_PUNCT_VARARGS = 0x103, // ...
    C_PUNCT_IADD    = 0x104, // +=
    C_PUNCT_ISUB    = 0x105, // -=
    C_PUNCT_IMUL    = 0x106, // *=
    C_PUNCT_IDIV    = 0x107, // /=
    C_PUNCT_IMOD    = 0x108, // %= 
    C_PUNCT_IAND    = 0x109, // &=
    C_PUNCT_IOR     = 0x10A, // |=
    C_PUNCT_IXOR    = 0x10B, // ^=
    C_PUNCT_INC     = 0x10C, // ++
    C_PUNCT_DEC     = 0x10D, // --
    C_PUNCT_RSHIFT  = 0x10E, // >>
    C_PUNCT_LSHIFT  = 0x10F, // <<
    C_PUNCT_LAND    = 0x110, // &&
    C_PUNCT_LOR     = 0x111, // ||
    C_PUNCT_EQ      = 0x112, // ==
    C_PUNCT_NEQ     = 0x113, // != 
    C_PUNCT_LEQ     = 0x114, // <= 
    C_PUNCT_GEQ     = 0x115, // >= 
    C_PUNCT_ARROW   = 0x116, // ->
    
    C_PUNCT_SENTINEL,
} c_punct;

// Enumeration of possible kinds of [[c_token.C_TOKEN_KW]]
typedef enum {
    C_PP_KEYWORD_DEFINE   = 0x1, // define
    C_PP_KEYWORD_UNDEF    = 0x2, // undef
    C_PP_KEYWORD_INCLUDE  = 0x3, // include
    C_PP_KEYWORD_IF       = 0x4, // if
    C_PP_KEYWORD_IFDEF    = 0x5, // ifdef
    C_PP_KEYWORD_IFNDEF   = 0x6, // ifndef
    C_PP_KEYWORD_ELSE     = 0x7, // else 
    C_PP_KEYWORD_ELIFDEF  = 0x8, // c2x elifdef
    C_PP_KEYWORD_ELIFNDEF = 0x9, // c2x elifndef
    C_PP_KEYWORD_PRAGMA   = 0xA, // pragma  
    C_PP_KEYWORD_ERROR    = 0xB, // error
    C_PP_KEYWORD_DEFINED  = 0xC, // defined
    C_PP_KEYWORD_LINE     = 0xD, // line
    C_PP_KEYWORD_ELIF     = 0xE, // elif
    C_PP_KEYWORD_ENDIF    = 0xF, // endif
    
    C_PP_KEYWORD_SENTINEL,
} c_pp_keyword;

// Token - atom in lexer
typedef struct c_token {
    // Kind of token
    c_token_kind kind;
    // Location
    c_lexer_loc loc;
    // Type paramters
    union {
        c_type    type;
        c_keyword kw;
        c_punct   punct;
    };
    // Type storage
    union {
        string   str;
        uint64_t u64;
        int64_t  i64;
        float    f32;
        double   f64;
    };
} c_token;

// Flags of [[c_pp_macro]]
enum {
    C_PP_MACRO_VARARGS_BIT       = 0x1,
    C_PP_MACRO_FUNCTION_LIKE_BIT = 0x2  
};

// Storage of individual preprocessor macro
typedef struct {
    // Hash of name
    str_hash hash;
    // Name of the macro
    string   name;
    // Flags
    uint8_t  flags;
    // Array of argument names.
    string  *arg_names;
    // Number of arguments
    uint32_t arg_count;
    // String containing definition
    string   definition;
    // Location of definition
    c_lexer_loc defined_at;
} c_pp_macro;

// Preprocessor is data structure storing data connected with preprocessor and managing it
typedef struct {
    // Where all strings from preprocessor are allocated
    // @NOTE: Because #undefs are a rare thing, we don't do any memory freeing of undef'ed argument names and definition
    // let's see if this becomes a problem 
    struct string_storage *ss;
    // Number of buckets in hash table
    // C standard defines strict number of maximum macros, so we can use inexpandable hash table with open adressing
    uint32_t     hash_size;
    // Hash table
    c_pp_macro *macro_hash;    
    // Stack of nested #ifs size
    uint32_t if_depth;
    // Marker whether current #if had true case and we should skip all further #elif's and #else's
    bool     is_current_if_handled;
} c_preprocessor;

// Initializes preprocessor 
void c_pp_init(c_preprocessor *pp, uint32_t hash_size);
// Push nested if
void c_pp_push_if(c_preprocessor *pp, bool is_handled);
// Pop nested if 
void c_pp_pop_if(c_preprocessor *pp);

// Basically add entry to macro hash table and return pointer to it. 
// NOTE: If macro of same name is already defined, do not warn about it
c_pp_macro *c_pp_define(c_preprocessor *pp, string name);
// Removes macro from hash table
// NOTE: Macro memory is not freed
void c_pp_undef(c_preprocessor *pp, string name);
// Returns macro from hash table if it exists, 0 otherwise
c_pp_macro *c_pp_get(c_preprocessor *pp, string name);

enum {
    c_lexbuf_FILE      = 0x1,  
    c_lexbuf_MACRO     = 0x2,  
    c_lexbuf_MACRO_ARG = 0x3,  
    c_lexbuf_CONCAT    = 0x4
};

// Structure describing tree of parsing c files
// Buffer is a text source of parsing, its main purpose is to maintain stack structure 
// of c file (like #include's or macro expansions)
// Each buffer is a separate source, that has (unique) text and some other properties
// After parsing, buffer is not used in lexing, but it can be used for error reporting
// This is why we save information about buffer to [[lexer_buffer_info]], that can be looked up
// in a hash table
typedef struct c_lexbuf {
    // Individual id of buffer
    c_lexbuf_id id;
    // Buffer, must be null-terminated
    // Must be correct UTF8 sequence
    uint8_t *buf;
    // Pointer where we are currently parsing
    uint8_t *at;
    // End pointer of [[buf]]
    // NOTE: Can be used to get size of buffer
    uint8_t *eof;
    // Kind of buffer
    uint8_t  kind;
    // Source location line, from 0
    uint32_t line;
    // Source location symbol, from 0
    uint32_t symb;
    // If file, id of file
    file_id file_id;
    // If macro expansion, macro which is being expanded
    c_pp_macro *macro;
    // If function-like macro expansion, strings of arguments passed to macro invocation 
    string *macro_args;
    // Linked list pointer
    struct lexer_buffer *next;
} c_lexbuf;

// Initialises base for lexbuf
void init_c_lexbuf(c_lexbuf *buf, void *buffer, uintptr_t buffer_size, uint8_t kind);
// Returns next byte of buffer, or 0 if end is reached
uint8_t c_lexbuf_peek(c_lexbuf *buffer);
// Try to advance cursor for n characters. Return number of characters advanced by
uint32_t c_lexbuf_advance(c_lexbuf *buffer, uint32_t n);
// Check if buffer contains lit at cursor
bool c_lexbuf_next_eq(c_lexbuf *buffer, string lit);
// If buffer contains lit at cursor, advance the cursor by length of lit
// Otherwise, do nothing
// Return result of c_lexbuf_next_eq
bool c_lexbuf_parse(c_lexbuf *buffer, string lit);

typedef struct lexer_buffer_info {
    c_lexbuf_id id;
    lexer_buffer_info *next;
} lexer_buffer_info;

// Wrapper for functionality and data connected with buffers
typedef struct {
    // Buffer stack
    c_lexbuf *current_buffer;
    // Number of elements in stack
    uint32_t  buffer_stack_size;
    // Value of next id - 1 (e.g. 0 means next id is 1)
    uint32_t id_cursor;
    // Freelist of buffers
    c_lexbuf *buffer_freelist;
} c_lexbuf_storage;

void init_c_lexbuf_storage(c_lexbuf_storage *bs);

#define C_LEXER_MAX_TOKEN_PEEK_DEPTH 16
 
typedef struct c_lexer {
    c_preprocessor pp;
    c_lexbuf_storage buffers;
    
    c_token  token_stack[C_LEXER_MAX_TOKEN_PEEK_DEPTH];
    uint32_t token_stack_at;
    
    struct file_registry *fr;
} c_lexer;

c_lexer *init_c_lexer(struct file_registry *fr, string filename);

c_token *c_lexer_peek_forward(c_lexer *lex, uint32_t forward);
c_token *c_lexer_peek(c_lexer *lex);
void c_lexer_eat(c_lexer *lex);

#endif