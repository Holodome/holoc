#include "parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "c_lang.h"
#include "c_types.h"
#include "error_reporter.h"
#include "hashing.h"
#include "llist.h"
#include "token_iter.h"

#define GET_DECLP(_scope, _hash)                                                             \
    hash_table_sc_get_u32((_scope)->decl_hash, ARRAY_SIZE((_scope)->decl_hash), parser_decl, \
                          next, name_hash, (_hash))

#define GET_TAGP(_scope, _hash)                                               \
    hash_table_sc_get_u32((_scope)->tag_hash, ARRAY_SIZE((_scope)->tag_hash), \
                          parser_tag_decl, next, name_hash, (_hash))

typedef enum {
    STORAGE_TYPEDEF_BIT      = 0x1,   // typedef
    STORAGE_STATIC_BIT       = 0x2,   // static
    STORAGE_EXTERN_BIT       = 0x4,   // extern
    STORAGE_INLINE_BIT       = 0x8,   // inline
    STORAGE_THREAD_LOCAL_BIT = 0x10,  // _Thread_local
} storage_class_specifier_flags;

c_type *parse_declspec(parser *p, uint32_t *storage_class_flags);

static parser_decl *
get_new_decl_scoped(parser *p, string name) {
    uint32_t name_hash = hash_string(name);
    assert(p->scope);
    parser_decl **declp = GET_DECLP(p->scope, name_hash);
    if (!*declp) {
        parser_decl *decl = calloc(1, sizeof(parser_decl));
        *declp            = decl;
    } else {
        NOT_IMPL;
    }

    parser_decl *decl = *declp;
    assert(decl);
    decl->name      = name;
    decl->name_hash = name_hash;
    return decl;
}

static void
push_tag_scoped(parser *p, string tag, c_type *type) {
    uint32_t tag_hash = hash_string(tag);
    assert(p->scope);

    parser_tag_decl **declp = GET_TAGP(p->scope, tag_hash);
    if (!*declp) {
        parser_tag_decl *decl = calloc(1, sizeof(parser_tag_decl));
        *declp                = decl;
    } else {
        NOT_IMPL;
    }

    parser_tag_decl *decl = *declp;
    assert(decl);
    decl->name      = tag;
    decl->name_hash = tag_hash;
    decl->type      = type;
}

static c_type *
find_typedef(parser *p, string name) {
    c_type *type       = NULL;
    uint32_t name_hash = hash_string(name);
    for (parser_scope *scope = p->scope; scope; scope = scope->next) {
        parser_tag_decl *decl = *GET_TAGP(scope, name_hash);
        if (decl) {
            type = decl->type;
            break;
        }
    }
    return type;
}

static int64_t
eval_const_expr(parser *p) {
    (void)p;
    NOT_IMPL;
    return 0;
}

static c_type *
parse_enum_decl(parser *p) {
    c_type *type = NULL;

    token *tok = ti_peek(p->it);
    if (!IS_KW(tok, C_KW_ENUM)) {
        goto out;
    }
    tok = ti_eat_peek(p->it);

    string tag = {0};
    if (tok->kind == TOK_ID) {
        tag = tok->str;
        tok = ti_eat_peek(p->it);
    }

    if (IS_PUNCT(tok, '{')) {
        tok = ti_eat_peek(p->it);

        uint64_t auto_value = 0;

        while (tok->kind != TOK_EOF && !IS_PUNCT(tok, '}')) {
            if (tok->kind != TOK_ID) {
                report_error_token(tok, "Expected identifier");
                tok = ti_eat_peek(p->it);
                continue;
            }

            string value_name = tok->str;

            tok            = ti_eat_peek(p->it);
            uint64_t value = auto_value;
            if (IS_PUNCT(tok, '=')) {
                tok   = ti_eat_peek(p->it);
                value = eval_const_expr(p);
            }
            auto_value = value + 1;

            parser_decl *decl = get_new_decl_scoped(p, value_name);
            decl->enum_val    = value;

            // TODO: Store enum values in type definition so we can do static analysis better
            // (like missing branches in switch statements)
        }

        if (tag.data) {
            push_tag_scoped(p, tag, type);
        }
    } else {
        NOT_IMPL;
    }

out:
    return type;
}

static c_type *
parse_struct_decl(parser *p) {
    c_type *type = NULL;

    token *tok = ti_peek(p->it);
    if (!IS_KW(tok, C_KW_STRUCT) && !IS_KW(tok, C_KW_UNION)) {
        goto out;
    }
    tok = ti_eat_peek(p->it);

    string tag = {0};
    if (tok->kind == TOK_ID) {
        tag = tok->str;
        tok = ti_eat_peek(p->it);
    }

    if (IS_PUNCT(tok, '{')) {
        linked_list_constructor members = {0};

        uint32_t idx = 0;

        while (tok->kind != TOK_EOF && !IS_PUNCT(tok, '}')) {
            uint32_t storage_class_flags = 0;

            c_type *decl_type  = parse_declspec(p, &storage_class_flags);
            bool first_in_decl = true;

            tok = ti_peek(p->it);
            // Anonymous struct member
            if ((decl_type->kind == C_TYPE_STRUCT || decl_type->kind == C_TYPE_UNION) &&
                IS_PUNCT(tok, ';')) {
                c_struct_member *member = calloc(1, sizeof(c_struct_member));
                member->type            = decl_type;
                member->idx             = idx++;
                LLISTC_ADD_LAST(&members, member);

                tok = ti_eat_peek(p->it);
                continue;
            }

            while (tok->kind != TOK_EOF && !IS_PUNCT(tok, ';')) {
                if (!first_in_decl) {
                    if (!IS_PUNCT(tok, ',')) {
                        report_error_token(tok, "Expected either ',' or ';'");
                    } else {
                        tok = ti_eat_peek(p->it);
                    }
                }
                first_in_decl = false;

                if (tok->kind != TOK_ID) {
                    report_error_token(tok, "Expected identifier");
                    tok = ti_eat_peek(p->it);
                    continue;
                }
                string name = tok->str;

                tok = ti_eat_peek(p->it);

                c_struct_member *member = calloc(1, sizeof(c_struct_member));
                member->type            = decl_type;
                member->name            = name;
                member->idx             = idx++;
                LLISTC_ADD_LAST(&members, member);
            }
        }

        type = make_c_type_struct(tag, members.first);

        if (tag.data) {
            push_tag_scoped(p, tag, type);
        }
    } else {
        NOT_IMPL;
    }

out:
    return type;
}

bool
token_is_typename(parser *p, token *tok) {
    bool result = false;
    if (tok->kind == TOK_KW) {
        switch (tok->kw) {
        default:
            break;
        case C_KW_AUTO:
        case C_KW_CHAR:
        case C_KW_CONST:
        case C_KW_DOUBLE:
        case C_KW_ENUM:
        case C_KW_EXTERN:
        case C_KW_FLOAT:
        case C_KW_INT:
        case C_KW_LONG:
        case C_KW_REGISTER:
        case C_KW_RESTRICT:
        case C_KW_RETURN:
        case C_KW_SHORT:
        case C_KW_SIGNED:
        case C_KW_STRUCT:
        case C_KW_TYPEDEF:
        case C_KW_UNION:
        case C_KW_UNSIGNED:
        case C_KW_VOID:
        case C_KW_VOLATILE:
        case C_KW_ATOMIC:
        case C_KW_BOOL:
        case C_KW_COMPLEX:
        case C_KW_DECIMAL32:
        case C_KW_DECIMAL64:
        case C_KW_DECIMAL128:
        case C_KW_THREAD_LOCAL:
            result = true;
            break;
        }
    }

    if (!result && tok->kind == TOK_ID) {
        result = find_typedef(p, tok->str) != 0;
    }
    return result;
}

// TODO: Pointer for storage_class_flags is ugly, put it into return struct
c_type *
parse_declspec(parser *p, uint32_t *storage_class_flags) {
    enum {
        TYPE_VOID_BIT  = 0x1,   // 1 void
        TYPE_BOOL_BIT  = 0x2,   // 1 _Bool
        TYPE_CHAR_BIT  = 0x4,   // 1 char
        TYPE_SHORT_BIT = 0x8,   // 1 short
        TYPE_INT_BIT   = 0x10,  // 1 int
        TYPE_LONG_BIT  = 0x20,  // 2 long
        /* reserved for long */
        TYPE_FLOAT_BIT    = 0x80,   // 1 float
        TYPE_DOUBLE_BIT   = 0x100,  // 1 double
        TYPE_OTHER_BIT    = 0x200,  // 1 non-builtin type
        TYPE_SIGNED_BIT   = 0x400,  // 1 signed
        TYPE_UNSIGNED_BIT = 0x800   // 1 unsigned
    };

    c_type *type = get_standard_type(C_TYPE_SINT);

    uint32_t type_flags = 0;

    token *tok = ti_peek(p->it);
    while (token_is_typename(p, tok)) {
        if (IS_KW(tok, C_KW_TYPEDEF) || IS_KW(tok, C_KW_STATIC) || IS_KW(tok, C_KW_INLINE) ||
            IS_KW(tok, C_KW_EXTERN) || IS_KW(tok, C_KW_THREAD_LOCAL)) {
            if (!storage_class_flags) {
                report_error_token(tok, "Declaration specifier is unexpected in this context");
                tok = ti_eat_peek(p->it);
                continue;
            }

            switch (tok->kw) {
                INVALID_DEFAULT_CASE;
            case C_KW_TYPEDEF:
                if (*storage_class_flags & STORAGE_TYPEDEF_BIT) {
                    report_error_token(tok, "Duplicate 'typedef' declaration specifier");
                }
                *storage_class_flags |= STORAGE_TYPEDEF_BIT;
                break;
            case C_KW_STATIC:
                if (*storage_class_flags & STORAGE_STATIC_BIT) {
                    report_error_token(tok, "Duplicate 'static' declaration specifier");
                }
                *storage_class_flags |= STORAGE_STATIC_BIT;
                break;
            case C_KW_INLINE:
                if (*storage_class_flags & STORAGE_INLINE_BIT) {
                    report_error_token(tok, "Duplicate 'inline' declaration specifier");
                }
                *storage_class_flags |= STORAGE_INLINE_BIT;
                break;
            case C_KW_EXTERN:
                if (*storage_class_flags & STORAGE_EXTERN_BIT) {
                    report_error_token(tok, "Duplicate 'extern' declaration specifier");
                }
                *storage_class_flags |= STORAGE_EXTERN_BIT;
                break;
            case C_KW_THREAD_LOCAL:
                if (*storage_class_flags & STORAGE_THREAD_LOCAL_BIT) {
                    report_error_token(tok, "Duplicate '_Thread_local' declaration specifier");
                }
                *storage_class_flags |= STORAGE_THREAD_LOCAL_BIT;
                break;
            }

            if ((*storage_class_flags & STORAGE_TYPEDEF_BIT) &&
                (*storage_class_flags & ~STORAGE_TYPEDEF_BIT)) {
                report_error_token(
                    tok,
                    "Declaration specifier 'typedef' can not be used with 'static', "
                    "'inline', 'extern', '_Thread_local'");
            }

            tok = ti_eat_peek(p->it);
            continue;
        }

        if (IS_KW(tok, C_KW_CONST) || IS_KW(tok, C_KW_VOLATILE) || IS_KW(tok, C_KW_AUTO) ||
            IS_KW(tok, C_KW_REGISTER) || IS_KW(tok, C_KW_RESTRICT) ||
            IS_KW(tok, C_KW_NORETURN)) {
            // TODO: do something
            tok = ti_eat_peek(p->it);
            continue;
        }

        if (IS_KW(tok, C_KW_STRUCT) || IS_KW(tok, C_KW_UNION)) {
            if (type_flags & TYPE_OTHER_BIT) {
                report_error_token(tok, "Duplicate non-builtin type");
            } else {
                type_flags |= TYPE_OTHER_BIT;
            }

            type = parse_struct_decl(p);
            tok  = ti_peek(p->it);
            continue;
        }

        if (IS_KW(tok, C_KW_ENUM)) {
            if (type_flags & TYPE_OTHER_BIT) {
                report_error_token(tok, "Duplicate non-builtin type");
            } else {
                type_flags |= TYPE_OTHER_BIT;
            }
            type = parse_enum_decl(p);
            tok  = ti_peek(p->it);
            continue;
        }

        if (tok->kind == TOK_ID) {
            if (type_flags & TYPE_OTHER_BIT) {
                report_error_token(tok, "Duplicate non-builtin type");
            } else {
                type_flags |= TYPE_OTHER_BIT;
            }

            c_type *id_type = find_typedef(p, tok->str);
            if (!id_type) {
                report_error_token(tok, "Undefined identifier");
            } else {
                type = id_type;
            }
            tok = ti_peek(p->it);
        }

        if (IS_KW(tok, C_KW_VOID)) {
            if (type_flags & TYPE_VOID_BIT) {
                report_error_token(tok, "Duplicate 'void' declaration specifier");
            } else {
                type_flags |= TYPE_VOID_BIT;
            }
        } else if (IS_KW(tok, C_KW_BOOL)) {
            if (type_flags & TYPE_BOOL_BIT) {
                report_error_token(tok, "Duplicate '_Bool' declaration specifier");
            } else {
                type_flags |= TYPE_BOOL_BIT;
            }
        } else if (IS_KW(tok, C_KW_CHAR)) {
            if (type_flags & TYPE_CHAR_BIT) {
                report_error_token(tok, "Duplicate 'char' declaration specifier");
            } else {
                type_flags |= TYPE_CHAR_BIT;
            }
        } else if (IS_KW(tok, C_KW_SHORT)) {
            if (type_flags & TYPE_SHORT_BIT) {
                report_error_token(tok, "Duplicate 'short' declaration specifier");
            } else {
                type_flags |= TYPE_SHORT_BIT;
            }
        } else if (IS_KW(tok, C_KW_INT)) {
            if (type_flags & TYPE_INT_BIT) {
                report_error_token(tok, "Duplicate 'int' declaration specifier");
            } else {
                type_flags |= TYPE_INT_BIT;
            }
        } else if (IS_KW(tok, C_KW_LONG)) {
            if (type_flags & (TYPE_LONG_BIT << 1)) {
                report_error_token(tok, "Excess 'long' declaration specifier");
            } else {
                type_flags |= TYPE_LONG_BIT;
            }
        } else if (IS_KW(tok, C_KW_FLOAT)) {
            if (type_flags & TYPE_FLOAT_BIT) {
                report_error_token(tok, "Duplicate 'float' declaration specifier");
            } else {
                type_flags |= TYPE_FLOAT_BIT;
            }
        } else if (IS_KW(tok, C_KW_DOUBLE)) {
            if (type_flags & TYPE_DOUBLE_BIT) {
                report_error_token(tok, "Duplicate 'double' declaration specifier");
            } else {
                type_flags |= TYPE_DOUBLE_BIT;
            }
        } else if (IS_KW(tok, C_KW_SIGNED)) {
            if (type_flags & TYPE_SIGNED_BIT) {
                report_error_token(tok, "Duplicate 'signed' declaration specifier");
            } else {
                type_flags |= TYPE_SIGNED_BIT;
            }
        } else if (IS_KW(tok, C_KW_UNSIGNED)) {
            if (type_flags & TYPE_UNSIGNED_BIT) {
                report_error_token(tok, "Duplicate 'unsigned' declaration specifier");
            } else {
                type_flags |= TYPE_UNSIGNED_BIT;
            }
        } else {
            UNREACHABLE;
        }

        switch (type_flags) {
        default:
            report_error_token(tok, "Invalid type");
            break;
        case TYPE_VOID_BIT:
            type = get_standard_type(C_TYPE_VOID);
            break;
        case TYPE_BOOL_BIT:
            type = get_standard_type(C_TYPE_BOOL);
            break;
        case TYPE_CHAR_BIT:
            type = get_standard_type(C_TYPE_CHAR);
            break;
        case TYPE_CHAR_BIT + TYPE_SIGNED_BIT:
            type = get_standard_type(C_TYPE_SCHAR);
            break;
        case TYPE_CHAR_BIT + TYPE_UNSIGNED_BIT:
            type = get_standard_type(C_TYPE_UCHAR);
            break;
        case TYPE_SHORT_BIT:
        case TYPE_SHORT_BIT + TYPE_INT_BIT:
        case TYPE_SIGNED_BIT + TYPE_SHORT_BIT:
        case TYPE_SIGNED_BIT + TYPE_SHORT_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_SSINT);
            break;
        case TYPE_UNSIGNED_BIT + TYPE_SHORT_BIT:
        case TYPE_UNSIGNED_BIT + TYPE_SHORT_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_USINT);
            break;
        case TYPE_INT_BIT:
        case TYPE_SIGNED_BIT:
        case TYPE_SIGNED_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_SINT);
            break;
        case TYPE_UNSIGNED_BIT:
        case TYPE_UNSIGNED_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_UINT);
            break;
        case TYPE_LONG_BIT:
        case TYPE_LONG_BIT + TYPE_INT_BIT:
        case TYPE_SIGNED_BIT + TYPE_LONG_BIT:
        case TYPE_SIGNED_BIT + TYPE_LONG_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_SLINT);
            break;
        case TYPE_UNSIGNED_BIT + TYPE_LONG_BIT:
        case TYPE_UNSIGNED_BIT + TYPE_LONG_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_ULINT);
            break;
        case TYPE_LONG_BIT + TYPE_LONG_BIT:
        case TYPE_LONG_BIT + TYPE_LONG_BIT + TYPE_INT_BIT:
        case TYPE_SIGNED_BIT + TYPE_LONG_BIT + TYPE_LONG_BIT:
        case TYPE_SIGNED_BIT + TYPE_LONG_BIT + TYPE_LONG_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_SLLINT);
            break;
        case TYPE_UNSIGNED_BIT + TYPE_LONG_BIT + TYPE_LONG_BIT:
        case TYPE_UNSIGNED_BIT + TYPE_LONG_BIT + TYPE_LONG_BIT + TYPE_INT_BIT:
            type = get_standard_type(C_TYPE_ULLINT);
            break;
        case TYPE_FLOAT_BIT:
            type = get_standard_type(C_TYPE_FLOAT);
            break;
        case TYPE_DOUBLE_BIT:
            type = get_standard_type(C_TYPE_DOUBLE);
            break;
        case TYPE_LONG_BIT + TYPE_DOUBLE_BIT:
            type = get_standard_type(C_TYPE_LDOUBLE);
            break;
        }

        tok = ti_eat_peek(p->it);
    }

    return type;
}

static c_type *
parse_type(parser *p) {
    c_type *type = NULL;
    (void)p;
    NOT_IMPL;
    return type;
}

uint64_t
evaluate_sizeof(parser *p) {
    (void)p;
    NOT_IMPL;
    return 0;
}

ast *
parse_funccall(parser *p) {
    ast *node = NULL;
    (void)p;
    NOT_IMPL;
    return node;
}

ast *
parse_expr_primary(parser *p) {
    ast *node = NULL;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '(')) {
        ti_eat(p->it);
        node = parse_expr(p);
        tok  = ti_peek(p->it);
        if (!IS_PUNCT(tok, ')')) {
            report_error_token(tok, "Missing closing paren");
        } else {
            ti_eat(p->it);
        }
    } else if (IS_KW(tok, C_KW_SIZEOF)) {
        source_loc loc = tok->loc;

        uint64_t sizeof_value = evaluate_sizeof(p);

        node = make_ast_num_int(loc, sizeof_value, get_standard_type(C_TYPE_ULLINT));
    } else if (IS_KW(tok, C_KW_ALIGNOF)) {
        NOT_IMPL;
    } else if (IS_KW(tok, C_KW_GENERIC)) {
        NOT_IMPL;
    } else if (tok->kind == TOK_ID) {
        NOT_IMPL;
    } else if (tok->kind == TOK_NUM) {
        if (c_type_is_int(tok->type)) {
            node = make_ast_num_int(tok->loc, tok->uint_value, tok->type);
        } else {
            node = make_ast_num_flt(tok->loc, tok->float_value, tok->type);
        }
        ti_eat(p->it);
    } else if (tok->kind == TOK_STR) {
        NOT_IMPL;
        /* } else { */
        /*     report_error_token(tok, "Expected expression"); */
    }
    return node;
}

ast *
parse_expr_postfix(parser *p) {
    ast *node = parse_expr_primary(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (tok->kind == TOK_ID) {
            node = parse_funccall(p);
        } else if (IS_PUNCT(tok, '[')) {
            // a[b] is alias for *(a + b)
            ti_eat(p->it);
            ast *idx = parse_expr(p);
            ast *add = make_ast_binary(AST_BIN_ADD, node, idx);
            node     = make_ast_unary(node->loc, AST_UN_DEREF, add);

            tok = ti_peek(p->it);
            if (!IS_PUNCT(tok, ']')) {
                report_error_token(tok, "Expected ']'");
            } else {
                tok = ti_eat_peek(p->it);
            }
        } else if (IS_PUNCT(tok, '.')) {
            NOT_IMPL;
        } else if (IS_PUNCT(tok, C_PUNCT_ARROW)) {
            NOT_IMPL;
        } else if (IS_PUNCT(tok, C_PUNCT_INC)) {
            ti_eat(p->it);

            node = make_ast_unary(node->loc, AST_UN_POSTINC, node);

            tok = ti_peek(p->it);
        } else if (IS_PUNCT(tok, C_PUNCT_DEC)) {
            ti_eat(p->it);

            node = make_ast_unary(node->loc, AST_UN_POSTDEC, node);

            tok = ti_peek(p->it);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_unary(parser *p) {
    ast *node = 0;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '+')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_PLUS, expr);
    } else if (IS_PUNCT(tok, '-')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_MINUS, expr);
    } else if (IS_PUNCT(tok, '!')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_LNOT, expr);
    } else if (IS_PUNCT(tok, '~')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_NOT, expr);
    } else if (IS_PUNCT(tok, '&')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_DEREF, expr);
    } else if (IS_PUNCT(tok, '*')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_ADDR, expr);
    } else if (IS_PUNCT(tok, C_PUNCT_INC)) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_PREINC, expr);
    } else if (IS_PUNCT(tok, C_PUNCT_DEC)) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(loc, AST_UN_PREDEC, expr);
    } else {
        node = parse_expr_postfix(p);
    }

    return node;
}

ast *
parse_expr_cast(parser *p) {
    ast *node = 0;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '(')) {
        source_loc loc = tok->loc;

        token *next = ti_peek_forward(p->it, 1);
        if (token_is_typename(p, next)) {
            ti_eat_multiple(p->it, 2);

            c_type *type = parse_type(p);

            tok = ti_peek(p->it);
            if (!IS_PUNCT(tok, ')')) {
                report_error_token(tok, "Expected ')'");
            } else {
                tok = ti_eat_peek(p->it);
            }

            ast *expr = parse_expr_cast(p);
            node      = make_ast_cast(loc, expr, type);
        }
    }

    if (node) {
        node = parse_expr_unary(p);
    }

    return node;
}

ast *
parse_expr_mul(parser *p) {
    ast *node = parse_expr_unary(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '*')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(AST_BIN_MUL, node, right);
        } else if (IS_PUNCT(tok, '/')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(AST_BIN_DIV, node, right);
        } else if (IS_PUNCT(tok, '%')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(AST_BIN_MOD, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_add(parser *p) {
    ast *node = parse_expr_mul(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '+')) {
            ti_eat(p->it);
            ast *right = parse_expr_mul(p);
            node       = make_ast_binary(AST_BIN_ADD, node, right);
        } else if (IS_PUNCT(tok, '-')) {
            ti_eat(p->it);
            ast *right = parse_expr_mul(p);
            node       = make_ast_binary(AST_BIN_SUB, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_shift(parser *p) {
    ast *node = parse_expr_add(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LSHIFT)) {
            ti_eat(p->it);
            ast *right = parse_expr_add(p);
            node       = make_ast_binary(AST_BIN_LSHIFT, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_RSHIFT)) {
            ti_eat(p->it);
            ast *right = parse_expr_add(p);
            node       = make_ast_binary(AST_BIN_RSHIFT, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_rel(parser *p) {
    ast *node = parse_expr_shift(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '<')) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(AST_BIN_L, node, right);
        } else if (IS_PUNCT(tok, '>')) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(AST_BIN_G, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_LEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(AST_BIN_LE, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_GEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(AST_BIN_GE, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_eq(parser *p) {
    ast *node = parse_expr_rel(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_EQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_rel(p);
            node       = make_ast_binary(AST_BIN_EQ, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_NEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_rel(p);
            node       = make_ast_binary(AST_BIN_NEQ, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_and(parser *p) {
    ast *node = parse_expr_eq(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '&')) {
            ti_eat(p->it);
            ast *right = parse_expr_eq(p);
            node       = make_ast_binary(AST_BIN_AND, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_xor(parser *p) {
    ast *node = parse_expr_and(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '^')) {
            ti_eat(p->it);
            ast *right = parse_expr_and(p);
            node       = make_ast_binary(AST_BIN_XOR, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_or(parser *p) {
    ast *node = parse_expr_xor(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '|')) {
            ti_eat(p->it);
            ast *right = parse_expr_xor(p);
            node       = make_ast_binary(AST_BIN_OR, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_land(parser *p) {
    ast *node = parse_expr_or(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LAND)) {
            ti_eat(p->it);
            ast *right = parse_expr_or(p);
            node       = make_ast_binary(AST_BIN_LAND, node, right);
        } else {
            break;
        }
        tok = ti_peek(p->it);
    }
    return node;
}

ast *
parse_expr_lor(parser *p) {
    ast *node = parse_expr_land(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LOR)) {
            ti_eat(p->it);
            ast *right = parse_expr_land(p);
            node       = make_ast_binary(AST_BIN_LOR, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_cond(parser *p) {
    ast *node = parse_expr_lor(p);

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '?')) {
        tok = ti_eat_peek(p->it);

        ast *cond_true = parse_expr(p);
        tok            = ti_peek(p->it);

        if (!IS_PUNCT(tok, ':')) {
            report_error_token(tok, "':' expected");
        } else {
            ti_eat(p->it);
        }

        ast *cond_false = parse_expr_cond(p);

        node = make_ast_ternary(node, cond_true, cond_false);
    }
    return node;
}

ast *
parse_expr_assign(parser *p) {
    ast *node = parse_expr_cond(p);

    token *tok = ti_peek(p->it);
    if (tok->kind == TOK_PUNCT) {
        switch (tok->punct) {
        default:
            break;
        case '=':
            node = make_ast_binary(AST_BIN_A, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IRSHIFT:
            node = make_ast_binary(AST_BIN_RSHIFTA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_ILSHIFT:
            node = make_ast_binary(AST_BIN_LSHIFTA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IADD:
            node = make_ast_binary(AST_BIN_ADDA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_ISUB:
            node = make_ast_binary(AST_BIN_SUBA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IMUL:
            node = make_ast_binary(AST_BIN_MULA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IDIV:
            node = make_ast_binary(AST_BIN_DIVA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IMOD:
            node = make_ast_binary(AST_BIN_MODA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IAND:
            node = make_ast_binary(AST_BIN_ANDA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IOR:
            node = make_ast_binary(AST_BIN_ORA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IXOR:
            node = make_ast_binary(AST_BIN_XORA, node, parse_expr_assign(p));
            break;
        }
    }

    return node;
}

ast *
parse_expr(parser *p) {
    ast *node = parse_expr_assign(p);

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, ',')) {
        node = make_ast_binary(AST_BIN_COMMA, node, parse_expr(p));
    }

    return node;
}

void
parse(parser *p) {
    for (;;) {
        ast *expr = parse_expr(p);
        if (!expr) {
            break;
        }

        char buffer[4096];
        fmt_ast_verbose(expr, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }
}
