#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "types.h"

struct pp_lexer;
struct bump_allocator;

#define PREPROCESSOR_MACRO_HASH_SIZE 8192

typedef struct preprocessor_token {
    uint32_t kind;
    uint32_t str_kind;
    uint32_t punct_kind;
    string str;
    struct preprocessor_token *next;
} preprocessor_token;

typedef struct preprocessor_macro_arg {
    string name;
    struct preprocessor_macro_arg *next;
} preprocessor_macro_arg;

typedef struct preprocessor_macro {
    string name;
    uint32_t name_hash;
    bool is_function_like;

    preprocessor_macro_arg *args;
    preprocessor_token *definition;
    struct preprocessor_macro *next;
} preprocessor_macro;

typedef struct preprocessor_conditional_include {
    bool is_included;
    bool is_after_else;
    struct preprocessor_conditional_include *next;
} preprocessor_conditional_include;

typedef enum token_kind {
    TOK_EOF = 0x0,
    TOK_ID  = 0x1,
    TOK_PUNCT = 0x2,
    TOK_STR = 0x3,
    TOK_NUM = 0x4,
} token_kind;

typedef struct token {
    token_kind kind;
    string str;
} token;

typedef struct token_list_entry {
    token tok;
    struct token_list_entry *next;
    struct token_list_entry *prev;
} token_list_entry;

// #pragma once files
// @NOTE: Number of files is generally small, so we are okay with having linked list 
// instead of hash map
typedef struct preprocessor_guarded_file {
    string name;
    struct preprocessor_guarded_file *next;
} preprocessor_guarded_file;

typedef struct preprocessor {
    struct pp_lexer *lexer;
    struct bump_allocator *a;
    struct allocator *ea;

    token_list_entry *token_list;

    // Value for __COUNTER__
    uint32_t counter_value; 
    preprocessor_conditional_include *cond_incl_stack;
    preprocessor_macro *macro_hash[PREPROCESSOR_MACRO_HASH_SIZE];
    preprocessor_guarded_file *included_files;

    preprocessor_macro *macro_freelist;
    preprocessor_macro_arg *macro_arg_freelist;
    preprocessor_token *tok_freelist;
    preprocessor_conditional_include *incl_freelist;
    token_list_entry *token_stack_freelist;
} preprocessor;
token preprocessor_get_token(preprocessor *pp);

#endif 
