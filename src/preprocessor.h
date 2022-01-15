#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "types.h"

struct pp_lexer;
struct bump_allocator;
struct pp_token;
struct c_type;

#define PREPROCESSOR_MACRO_HASH_SIZE 8192

typedef struct pp_macro_arg {
    string name;
    struct pp_macro_arg *next;
} pp_macro_arg;

typedef enum {
    PP_MACRO_REG       = 0x0,
    PP_MACRO_FILE      = 0x1,  // __FILE__
    PP_MACRO_LINE      = 0x2,  // __LINE__
    PP_MACRO_COUNTER   = 0x3,  // __COUNTER__
    PP_MACRO_TIMESTAMP = 0x4,  // __TIMESTAMP__
    PP_MACRO_BASE_FILE = 0x5,  // __BASE_FILE__
    PP_MACRO_DATE      = 0x6,  // __DATE__
    PP_MACRO_TIME      = 0x7,  // __TIME__
} pp_macro_kind;

typedef struct pp_macro {
    pp_macro_kind kind;
    string name;
    uint32_t name_hash;
    bool is_function_like;

    pp_macro_arg *args;
    struct pp_token *definition;
    struct pp_macro *next;
} pp_macro;

typedef struct pp_conditional_include {
    bool is_included;
    bool is_after_else;
    struct pp_conditional_include *next;
} pp_conditional_include;

typedef enum token_kind {
    TOK_EOF   = 0x0,  // End of file
    TOK_ID    = 0x1,  // Identfier
    TOK_PUNCT = 0x2,  // Punctuator
    TOK_STR   = 0x3,  // String or arbitrary type
    TOK_NUM   = 0x4,  // Number (can be numeric literal or char literal)
    TOK_KW    = 0x5,  // Keyword
} token_kind;

typedef struct token {
    token_kind kind;
    string str;
    int64_t int_value;
    long double float_value;
    struct c_type *type;
} token;

// #pragma once files
// @NOTE: Number of files is generally small, so we are okay with having linked
// list instead of hash map
typedef struct pp_guarded_file {
    string name;
    struct pp_guarded_file *next;
} pp_guarded_file;

typedef struct pp_macro_expansion_arg {
    struct p_token *tokens;
    struct pp_macro_expansion_arg *next;
} pp_macro_expansion_arg;

typedef struct preprocessor {
    struct pp_lexer *lex;
    struct bump_allocator *a;
    struct allocator *ea;

    // Value for __COUNTER__
    uint32_t counter_value;
    pp_conditional_include *cond_incl_stack;
    pp_macro *macro_hash[PREPROCESSOR_MACRO_HASH_SIZE];
    pp_guarded_file *included_files;

    pp_macro *macro_freelist;
    pp_macro_arg *macro_arg_freelist;
    struct pp_token *tok_freelist;
    pp_conditional_include *incl_freelist;
    pp_macro_expansion_arg *macro_expansion_arg_freelist;
} preprocessor;

void do_pp(preprocessor *pp);

uint32_t fmt_token(token *tok, char *buf, uint32_t buf_size);
uint32_t fmt_token_verbose(token *tok, char *buf, uint32_t buf_size);

#endif
