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
    PP_MACRO_OBJ       = 0x0,
    PP_MACRO_FUNC      = 0x1,
    PP_MACRO_FILE      = 0x2,  // __FILE__
    PP_MACRO_LINE      = 0x3,  // __LINE__
    PP_MACRO_COUNTER   = 0x4,  // __COUNTER__
    PP_MACRO_TIMESTAMP = 0x5,  // __TIMESTAMP__
    PP_MACRO_BASE_FILE = 0x6,  // __BASE_FILE__
    PP_MACRO_DATE      = 0x7,  // __DATE__
    PP_MACRO_TIME      = 0x8,  // __TIME__
} pp_macro_kind;

typedef struct pp_macro {
    pp_macro_kind kind;
    string name;
    uint32_t name_hash;

    pp_macro_arg *args;
    uint32_t definition_len;
    struct pp_token *definition;
    struct pp_macro *next;
} pp_macro;

typedef struct pp_conditional_include {
    bool is_included;
    bool is_after_else;
    struct pp_conditional_include *next;
} pp_conditional_include;
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

#endif
