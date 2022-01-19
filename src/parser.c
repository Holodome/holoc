#include "parser.h"

#include <assert.h>

#include "ast.h"
#include "ast_builder.h"

static void 
parse_typedef(parser *p, ast_typedef *td) {
    NOT_IMPL;
}

void
whatever(parser *p) {
    ast *toplevel = build_toplevel_ast(p->b);
    switch (toplevel->kind) {
    case AST_TYPEDEF:
        parse_typedef(p, (ast_typedef *)toplevel);
        break;
    case AST_FUNC:
    NOT_IMPL;
        break;
    case AST_STRUCT:
    NOT_IMPL;
        break;
    case AST_ENUM:
    NOT_IMPL;
        break;
    default:
        assert(false);
        break;
    }
}
