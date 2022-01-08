/*
Author: Holodome
Date: 16.21.2021
File: src/preprocessor.h
Version: 0

This is the first stage from which compilation takes place.
Because C language on user level is a composition of two different languages,
it is benefitial to split them implementation-wise too for better understanding 
and functionality separation

Preprocessor takes stream of utf8 texts and parses them into c tokens
To make things simples, preprocessor does not ouput c tokens directly
Instead, it does only the work connected with parsing strings 
and outputs string with expected token kind so the lexer can form 
correct token from it
*/
#include "types.h"

// Types of character literals
typedef enum { 
    C_PP_CHAR_LIT_REG     = 0x1, // '
    C_PP_CHAR_LIT_UTF8    = 0x2, // u8'
    C_PP_CHAR_LIT_UTF16   = 0x3, // u'
    C_PP_CHAR_LIT_UTF32   = 0x4, // U'
    C_PP_CHAR_LIT_WIDE    = 0x5,  // L'
    C_PP_STRING_LIT_REG   = 0x10, // "
    C_PP_STRING_LIT_UTF8  = 0x20, // u8"
    C_PP_STRING_LIT_UTF16 = 0x30, // u"
    C_PP_STRING_LIT_UTF32 = 0x40, // U"
    C_PP_STRING_LIT_WIDE  = 0x50, // L"

    C_PP_STRING_LIT_CHAR_MASK   = 0xF,
    C_PP_STRING_LIT_STRING_MASK = 0xF0,
} pp_string_lit_kind;

// Types of preprocessor tokens
typedef enum {
    C_PP_TOKEN_EOF   = 0x0,
    C_PP_TOKEN_INT   = 0x1,
    C_PP_TOKEN_REAL  = 0x2,
    C_PP_TOKEN_STR   = 0x3,
    C_PP_TOKEN_IDENT = 0x4,
    C_PP_TOKEN_PUNCT = 0x5,
} pp_token_kind;

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
} c_pp_macro;

// Basically add entry to macro hash table and return pointer to it. 
// NOTE: If macro of same name is already defined, do not warn about it
c_pp_macro *c_pp_define(c_preprocessor *pp, string name);
// Removes macro from hash table
// NOTE: Macro memory is not freed
void c_pp_undef(c_preprocessor *pp, string name);
// Returns macro from hash table if it exists, 0 otherwise
c_pp_macro *c_pp_get(c_preprocessor *pp, string name);

enum {
    PP_LEXBUF_FILE      = 0x1,  
    PP_LEXBUF_MACRO     = 0x2,  
    PP_LEXBUF_MACRO_ARG = 0x3,  
    PP_LEXBUF_CONCAT    = 0x4
};

// Structure describing tree of parsing c files
// Buffer is a text source of parsing, its main purpose is to maintain stack structure 
// of c file (like #include's or macro expansions)
// Each buffer is a separate source, that has (unique) text and some other properties
// After parsing, buffer is not used in lexing, but it can be used for error reporting
// This is why we save information about buffer to [[lexer_buffer_info]], that can be looked up
// in a hash table
typedef struct pp_lexbuf {
    // Individual id of buffer
    pp_lexbuf_id id;
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
} pp_lexbuf;

// Initialises base for lexbuf
void init_pp_lexbuf(pp_lexbuf *buf, void *buffer, uintptr_t buffer_size, uint8_t kind);
// Returns next byte of buffer, or 0 if end is reached
uint8_t pp_lexbuf_peek(pp_lexbuf *buffer);
// Try to advance cursor for n characters. Return number of characters advanced by
uint32_t pp_lexbuf_advance(pp_lexbuf *buffer, uint32_t n);
// Check if buffer contains lit at cursor
bool pp_lexbuf_next_eq(pp_lexbuf *buffer, string lit);
// If buffer contains lit at cursor, advance the cursor by length of lit
// Otherwise, do nothing
// Return result of pp_lexbuf_next_eq
bool pp_lexbuf_parse(pp_lexbuf *buffer, string lit);

typedef struct lexer_buffer_info {
    pp_lexbuf_id id;
    struct lexer_buffer_info *next;
} lexer_buffer_info;

// Wrapper for functionality and data connected with buffers
typedef struct {
    // Buffer stack
    pp_lexbuf *current_buffer;
    // Number of elements in stack
    uint32_t  buffer_stack_size;
    // Value of next id - 1 (e.g. 0 means next id is 1)
    uint32_t id_cursor;
    // Freelist of buffers
    pp_lexbuf *buffer_freelist;
} pp_lexbuf_storage;

// Initializes [[pp_lexbuf_storage]]
void init_pp_lexbuf_storage(pp_lexbuf_storage *bs);
// Returns new pp_lexbuf and pushes it in stack
pp_lexbuf *pp_lexbuf_storage_add(pp_lexbuf_storage *bs);
// Removes top of the stack buffer
void pp_lexbuf_storage_remove(pp_lexbuf_storage *bs);

// Preprocessor is data structure storing data connected with preprocessor and managing it
typedef struct {
    struct bump_allocator *allocator;

    pp_lexbuf_storage bs;
    // Where all strings from preprocessor are allocated
    // @NOTE: Because #undefs are a rare thing, we don't do any memory freeing of undef'ed argument names and definition
    // let's see if this becomes a problem 
    struct string_storage *ss;
    // Number of buckets in hash table
    // C standard defines strict number of maximum macros, so we can use inexpandable hash table with open adressing
    uint32_t     macro_hash_size;
    // Hash table
    c_pp_macro *macro_hash;    
    // Stack of nested #ifs size
    uint32_t if_depth;
    // Marker whether current #if had true case and we should skip all further #elif's and #else's
    bool is_current_if_handled;

    // Intermediate storage for tokens before they are parsed
    char token_buffer[4096];
    pp_token_kind   token_kind;
    char_lit_kind   token_char_kind;
    string_lit_kind token_string_kind;
} c_preprocessor;

// Initializes preprocessor 
void c_pp_init(c_preprocessor *pp, uint32_t hash_size);
// Push nested if
void c_pp_push_if(c_preprocessor *pp, bool is_handled);
// Pop nested if 
void c_pp_pop_if(c_preprocessor *pp);

void c_pp_parse(c_preprocessor *pp);
