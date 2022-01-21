#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "types.h"

struct pp_lexer;
struct bump_allocator;
struct pp_token;
struct c_type;

#define PREPROCESSOR_MACRO_HASH_SIZE 2048

typedef struct pp_macro_arg {
    struct pp_macro_arg *next;
    // Name of the argument (__VA_ARGS__ for variadic arguments)
    string name;
    // Because we know that macro can only expand itself one time (there is no
    // recursive expansion we can use this hack to store macro argument
    // invocation tokens. These three fields are populated during expand_macro
    // function, and cleared afterwards
    uint32_t tok_count;
    struct pp_token *toks;
} pp_macro_arg;

typedef enum {
    // Object-like macro
    PP_MACRO_OBJ = 0x0,
    // Function-like macro
    PP_MACRO_FUNC = 0x1,
    // NOTE: These macros generated depending on location in source code,
    // so their definition differs. That's why they are put in their own macro
    // kinds
    // __FILE__
    PP_MACRO_FILE = 0x2,
    // __LINE__
    PP_MACRO_LINE = 0x3,
    // __COUNTER__
    PP_MACRO_COUNTER = 0x4,
    // __TIMESTAMP__
    PP_MACRO_TIMESTAMP = 0x5,
    // __BASE_FILE__
    PP_MACRO_BASE_FILE = 0x6,
    // __DATE__
    PP_MACRO_DATE = 0x7,
    // __TIME__
    PP_MACRO_TIME = 0x8,
} pp_macro_kind;

typedef struct pp_macro {
    struct pp_macro *next;

    pp_macro_kind kind;
    string name;
    uint32_t name_hash;

    bool is_variadic;
    uint32_t arg_count;
    pp_macro_arg *args;
    uint32_t definition_len;
    struct pp_token *definition;
} pp_macro;

typedef struct pp_conditional_include {
    bool is_included;
    bool is_after_else;
    struct pp_conditional_include *next;
} pp_conditional_include;

typedef struct pp_macro_expansion_arg {
    struct p_token *tokens;
    struct pp_macro_expansion_arg *next;
} pp_macro_expansion_arg;

typedef struct pp_parse_stack {
    struct pp_parse_stack *next;
    struct file *file;
    struct pp_lexer *lexer;
} pp_parse_stack;

typedef struct preprocessor {
    struct bump_allocator *a;
    struct allocator *ea;

    struct file_storage *fs;
    char lexer_buffer[4096];

    // Value for __COUNTER__
    uint32_t counter_value;
    pp_conditional_include *cond_incl_stack;
    pp_macro *macro_hash[PREPROCESSOR_MACRO_HASH_SIZE];
    pp_parse_stack *parse_stack;

    pp_macro *macro_freelist;
    pp_macro_arg *macro_arg_freelist;
    struct pp_token *tok_freelist;
    pp_conditional_include *cond_incl_freelist;
    pp_macro_expansion_arg *macro_expansion_arg_freelist;
    pp_parse_stack *parse_stack_freelist;
} preprocessor;

struct token *do_pp(preprocessor *pp, string name);

#endif
