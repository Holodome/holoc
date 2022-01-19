#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "types.h"

struct bump_allocator;
struct ast;
struct allocator;
struct token;

typedef struct ast_builder {
    struct bump_allocator *a;
    struct allocator *ea;

    struct token *toks;
    struct token *tok;

    struct ast *last_toplevel_ast;
    struct ast *ast_freelist;
} ast_builder;

struct ast *build_toplevel_ast(ast_builder *b);

#endif 
