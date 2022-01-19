#include "ast_builder.h"

#include <assert.h>

#include "ast.h"
#include "c_lang.h"
#include "c_types.h"
#include "llist.h"
#include "bump_allocator.h"

typedef struct {
    ast *node;
} ast_walker;

static ast_walker
walk_ast(ast *node) {
    NOT_IMPL;
    return (ast_walker){0};
}

static bool
walker_valid(ast_walker *w) {
    NOT_IMPL;
    return false;
}

static void
walker_next(ast_walker *w) {
    NOT_IMPL;
}

static void *
make_ast(ast_builder *b, ast_kind kind) {
    ast *node = bump_alloc(b->a, sizeof(ast));
    node->kind = kind;
    return node;
}

static ast *
build_enum_decl(ast_builder *b) {
    ast *node = 0;
    NOT_IMPL;
    return node;
}

static ast *
build_struct_decl(ast_builder *b) {
    ast *node = 0;
    NOT_IMPL;
    return node;
}

static ast *
build_typedef(ast_builder *b) {
    ast *node = 0;

    assert(IS_KW(b->tok, C_KW_TYPEDEF));
    b->tok = b->tok->next;

    if (IS_KW(b->tok, C_KW_ENUM)) { 
        // Parse typedef enum {} a;
        b->tok = b->tok->next;
        ast *enum_decl = build_enum_decl(b);
        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying = enum_decl;
        td->name = typedef_name;

        node = (ast *)td;
    } else if (IS_KW(b->tok, C_KW_STRUCT) || IS_KW(b->tok, C_KW_UNION)) {
        // parse typedef struct {} a;
        b->tok = b->tok->next;
        ast *struct_decl = build_struct_decl(b);

        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying = struct_decl;
        td->name = typedef_name;

        node = (ast *)td;
    } else if (b->tok->kind == TOK_ID) {
        // parse typedef int my_type;
    } else if (b->tok->kind == TOK_KW) {
        // parse typedef unsigned long long int my_int;
    }

    assert(b->tok->kind == TOK_PUNCT && b->tok->punct == ';');
    b->tok = b->tok->next;

    return node;
}

ast *
build_toplevel_ast(ast_builder *b) {
    for (ast_walker walker = walk_ast(b->last_toplevel_ast);
         walker_valid(&walker); walker_next(&walker)) {
        LLIST_ADD(b->ast_freelist, walker.node);
    }

    ast *node = 0;
    if (IS_KW(b->tok, C_KW_TYPEDEF)) {
        node = build_typedef(b);
    }

    return node;
}
