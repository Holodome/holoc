// This file contains code for what is essentially a tool for iterating
// preprocessor token stream. In many compilers parsing consits of operating on
// such stream, which can be defined with public interface (peek, eat). Here
// peek returns next tokens until it is eaten, then skips to the next. Such
// stream is benefitial in terms of memory and can be described as lazy
// algorithm, opposed to generating tokens before parsing.
//
// The compiler is designed in a way that there should be minimal number of
// stray types across compilation values, so all information that persists
// should be stored outside on parsing types (like pp_token, token, ast).
//
// Stream of preprocessor tokens is unique because of the nature of the C
// language, which supports including of files. Including essentially means
// 'copy all the source code and paste it here'. This functionality is
// implemented using a stack for maintating includes. This way, new include is
// appended on top of last and does not touch state of later.
// ppti_entry is entry of this stack.
//
// Issues come when we want to think about memory. Logically, the only resource
// that token iterarator is responsible for are its stack entries. But it is
// also a place of allocating and storing pp_lexer's and pp_token's. It has been
// decided to use freelists for these types, which can't be placed here because
// allocation my occure elsewhere in preprocessor. So we use pointers to it
// here.
#ifndef PP_TOKEN_ITERATOR_H
#define PP_TOKEN_ITERATOR_H

#include "types.h"

struct pp_token;
struct pp_lexer;
struct file;
struct allocator;
struct file_storage;

// Entry of preprocessor parse stack.
typedef struct ppti_entry {
    struct ppti_entry *next;
    // Linked list of tokens. Peeked tokens are stored here, and put to freelist
    // after eating.
    struct pp_token *token_list;
    // If this ppti_entry is a file, lexer for that file.
    struct pp_lexer *lexer;
    // Must be present if lexer is present.
    struct file *f;
} ppti_entry;

// Structure holding state information about token parsing.
typedef struct pp_token_iter {
    struct allocator *a;

    char *tok_buf;
    uint32_t tok_buf_capacity;
    uint32_t tok_buf_len;

    ppti_entry *it;

    struct pp_token *eof_token;

    struct pp_token **token_freelist;
    struct pp_lexer **lexer_freelist;
    struct ppti_entry *it_freelist;
} pp_token_iter;

void ppti_include_file(pp_token_iter *it, struct file_storage *fs,
                       string filename);
void ppti_insert_tok_list(pp_token_iter *it, struct pp_token *first,
                          struct pp_token *last);

struct pp_token *ppti_peek_forward(pp_token_iter *it, uint32_t count);
struct pp_token *ppti_peek(pp_token_iter *it);

void ppti_eat_multiple(pp_token_iter *it, uint32_t count);
void ppti_eat(pp_token_iter *it);

struct pp_token *ppti_eat_peek(pp_token_iter *it);

#endif
