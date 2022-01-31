#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "types.h"

struct pp_lexer;
struct bump_allocator;
struct pp_token;
struct c_type;
struct token;
struct pp_token_iter;

#define PREPROCESSOR_MACRO_HASH_SIZE 2048

typedef struct pp_macro_arg {
    struct pp_macro_arg *next;
    // Name of the argument (__VA_ARGS__ for variadic arguments)
    string name;
    // Linked list of expansion. Terminated with EOF
    struct pp_token *toks;
} pp_macro_arg;

typedef enum {
    // Object-like macro
    PP_MACRO_OBJ = 0x1,
    // Function-like macro
    PP_MACRO_FUNC = 0x2,
    // NOTE: These macros generated depending on location in source code,
    // so their definition differs. That's why they are put in their own macro
    // kinds
    // __FILE__
    PP_MACRO_FILE = 0x3,
    // __LINE__
    PP_MACRO_LINE = 0x4,
    // __COUNTER__
    PP_MACRO_COUNTER = 0x5,
    // __INCLUDE_LEVEL__
    PP_MACRO_INCLUDE_LEVEL = 0x6,
} pp_macro_kind;

// Container for macros
typedef struct pp_macro {
    struct pp_macro *next;
    // Name (like _NDBEBUG)
    string name;
    // Used in hash table
    uint32_t name_hash;

    pp_macro_kind kind;
    // If function-like
    bool is_variadic;
    uint32_t arg_count;
    pp_macro_arg *args;
    // Linked list of definition. Terminated with EOF
    struct pp_token *definition;
} pp_macro;

// Stores inofromation about conditional include stack (#if's)
typedef struct pp_conditional_include {
    struct pp_conditional_include *next;
    // If some part of code was parsed. Means that others shouldn't
    bool is_included;
    // Used to check if #else was met, which means that #elif's are no longer
    // accepted
    bool is_after_else;
} pp_conditional_include;

typedef struct preprocessor {
    // Allocator used for allocating objects needed for parsing.
    // Preprocessor is designed in a way that thries to minimize number of
    // deallocations.
    struct allocator *a;
    //
    struct file_storage *fs;

    struct pp_token_iter *it;
    // Buffer where strings and other normally heap-allocated string-like data
    // is written. Because we want to simplify preprocessor structure a bit, we
    // don't request for a specific allocator. Instead, strings are written to
    // this buffer and user may decide how to use it.
    char *tok_buf;
    uint32_t tok_buf_capacity;
    uint32_t tok_buf_len;
    // Value for __COUNTER__
    uint32_t counter_value;
    // Stack of conditional includes. Pointer because default level is not an
    // include.
    pp_conditional_include *cond_incl_stack;
    // Macro hash table
    pp_macro *macro_hash[PREPROCESSOR_MACRO_HASH_SIZE];

    pp_macro *macro_freelist;
    pp_macro_arg *macro_arg_freelist;
    pp_conditional_include *cond_incl_freelist;
    struct pp_token *tok_freelist;
    struct pp_lexer *lex_freelist;
} preprocessor;

void init_pp(preprocessor *pp, string filename, char *tok_buf,
             uint32_t tok_buf_size);
bool pp_parse(preprocessor *pp, struct token *tok);

#endif
