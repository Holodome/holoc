#include "ast_builder.h"

#if 0
#include <assert.h>

#include "ast.h"
#include "ast_builder.h"
#include "bump_allocator.h"
#include "c_lang.h"
#include "c_types.h"
#include "llist.h"

static void *
make_ast(ast_builder *b, ast_kind kind) {
    assert(kind < ARRAY_SIZE(AST_STRUCT_SIZES));
    ast *node  = bump_alloc(b->a, AST_STRUCT_SIZES[kind]);
    node->kind = kind;
    return node;
}

static ast *
make_ident(ast_builder *b, string ident) {
    ast_identifier *node = make_ast(b, AST_ID);
    node->ident          = ident;
    return (ast *)node;
}

static ast *
build_builtin_type(ast_builder *b) {
    ast *node = NULL;
    NOT_IMPL;
    return node;
}

static uint64_t
evaluate_constant_expression(ast_builder *b) {
    NOT_IMPL;
    return 0;
}

static ast *
build_enum_decl(ast_builder *b) {
    ast *node = NULL;
    assert(IS_KW(b->tok, C_KW_ENUM));
    b->tok = b->tok->next;

    string enum_name = {0};
    if (b->tok->kind == TOK_ID) {
        enum_name = b->tok->str;
        b->tok    = b->tok->next;
    }

    if (IS_PUNCT(b->tok, '{')) {
        b->tok = b->tok->next;

        uint64_t auto_value            = 0;
        linked_list_constructor values = {0};

        while (!IS_PUNCT(b->tok, '}')) {
            assert(b->tok->kind == TOK_ID);

            string value_name = b->tok->str;
            b->tok            = b->tok->next;
            uint64_t value    = auto_value;
            if (IS_PUNCT(b->tok, '=')) {
                b->tok = b->tok->next;
                uint64_t constant_expression_value =
                    evaluate_constant_expression(b);
                value = constant_expression_value;
            }
            auto_value = value + 1;

            if (IS_PUNCT(b->tok, ',')) {
                b->tok = b->tok->next;
            }

            ast_enum_field *field = make_ast(b, AST_ENUM_VALUE);
            field->name           = value_name;
            field->value          = value;
            LLISTC_ADD_LAST(&values, field);
        }

        ast_enum_decl *decl = make_ast(b, AST_ENUM);
        decl->name          = enum_name;
        decl->fields        = values.first;
        node                = (ast *)decl;
    } else {
        NOT_IMPL;
    }

    NOT_IMPL;
    return node;
}

static ast *
build_struct_decl(ast_builder *b) {
    ast *node = NULL;
    NOT_IMPL;
    return node;
}

static ast *
build_typedef(ast_builder *b) {
    ast *node = NULL;

    assert(IS_KW(b->tok, C_KW_TYPEDEF));
    b->tok = b->tok->next;

    if (IS_KW(b->tok, C_KW_ENUM)) {
        // Parse typedef enum {} a;
        b->tok         = b->tok->next;
        ast *enum_decl = build_enum_decl(b);
        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying  = enum_decl;
        td->name        = typedef_name;

        node = (ast *)td;
    } else if (IS_KW(b->tok, C_KW_STRUCT) || IS_KW(b->tok, C_KW_UNION)) {
        // parse typedef struct {} a;
        b->tok           = b->tok->next;
        ast *struct_decl = build_struct_decl(b);

        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying  = struct_decl;
        td->name        = typedef_name;

        node = (ast *)td;
    } else if (b->tok->kind == TOK_ID) {
        string underlying_name = b->tok->str;
        b->tok                 = b->tok->next;

        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying  = make_ident(b, underlying_name);
        td->name        = typedef_name;

        node = (ast *)td;
        // parse typedef my_int my_type;
    } else if (b->tok->kind == TOK_KW) {
        // parse typedef unsigned long long int my_int;
        ast *underlying_type = build_builtin_type(b);
        assert(underlying_type);

        assert(b->tok->kind == TOK_ID);
        string typedef_name = b->tok->str;

        ast_typedef *td = make_ast(b, AST_TYPEDEF);
        td->underlying  = underlying_type;
        td->name        = typedef_name;

        node = (ast *)td;
    } else {
        NOT_IMPL;
    }

    assert(b->tok->kind == TOK_PUNCT && b->tok->punct == ';');
    b->tok = b->tok->next;

    return node;
}

ast *
build_toplevel_ast(ast_builder *b) {
    ast *node = NULL;
    if (IS_KW(b->tok, C_KW_TYPEDEF)) {
        node = build_typedef(b);
    }

    return node;
}
#endif
