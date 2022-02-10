#ifndef PARSER_H
#define PARSER_H

#include "general.h"

#define PARSER_SCOPE_VAR_HASH_SIZE 1024
#define PARSER_SCOPE_TAG_HASH_SIZE 1024

struct token_iter;
struct ast;

typedef enum {
    PARSER_DECL_TYPEDEF  = 0x1,
    PARSER_DECL_VAR      = 0x2,
    PARSER_DECL_FUNC     = 0x3,
    PARSER_DECL_ENUM_VAL = 0x4,
} parser_decl_kind;

typedef struct parser_decl {
    struct parser_decl *next;

    string name;
    uint32_t name_hash;

    parser_decl_kind kind;
    struct c_type *type;
    uint64_t enum_val;
} parser_decl;

typedef struct parser_tag_decl {
    struct parser_tag_decl *next;

    string name;
    uint32_t name_hash;
    struct c_type *type;
} parser_tag_decl;

typedef struct parser_scope {
    struct parser_scope *next;

    parser_decl *decl_hash[PARSER_SCOPE_VAR_HASH_SIZE];
    parser_tag_decl *tag_hash[PARSER_SCOPE_TAG_HASH_SIZE];
} parser_scope;

typedef struct parser {
    struct token_iter *it;

    parser_scope *scope;
} parser;

// primary = '(' expr ')'
//         | 'sizeof' '(' type_name ')'
//         | 'sizeof' unary
//         | '_Alignof' '(' type_name ')'
//         | '_Alignof' unary
//         | '_Generic' generic_selection
//         | ident
//         | str
//         | num
struct ast *parse_expr_primary(parser *p);
// postfix = '(' type_name ')' '{' initializer_list '}'
//         | ident '(' func_args ')' postfix_tail*
//         | primary postfix_tail*
//
// postfix_tail = '[' expr ']'
//              | '(' func_args ')'
//              | '.' ident
//              | '->' ident
//              | '++'
//              | '--'
struct ast *parse_expr_postfix(parser *p);
// unary = '+' cast
//       | '-' cast
//       | '!' cast
//       | '~' cast
//       | '&' cast
//       | '*' cast
//       | '++' unary
//       | '--' unary
//       | postfix
struct ast *parse_expr_unary(parser *p);
// cast = '(' type_name ')' cast | unary
struct ast *parse_expr_cast(parser *p);
// mul = unary ('*' unary |
//              '/' unary |
//              '%' unary)*
struct ast *parse_expr_mul(parser *p);
// add = mul ('+' mul |
//            '-' mul)*
struct ast *parse_expr_add(parser *p);
// shift = add ('<<' add |
//              '>>' add)*
struct ast *parse_expr_shift(parser *p);
// rel = shift ('<=' shift |
//              '<'  shift |
//              '>'  shift |
//              '>=' shift)*
struct ast *parse_expr_rel(parser *p);
// eq = rel ('==' rel | '!=' rel)*
struct ast *parse_expr_eq(parser *p);
// and = eq ('&' eq)*
struct ast *parse_expr_and(parser *p);
// xor = and ('^' and)*
struct ast *parse_expr_xor(parser *p);
// or = xor ('|' xor)*
struct ast *parse_expr_or(parser *p);
// land = or ('&&' or)*
struct ast *parse_expr_land(parser *p);
// lor = land ('||' land)*
struct ast *parse_expr_lor(parser *p);
// cond = lor ('?' expr ':' cond)?
struct ast *parse_expr_cond(parser *p);
// assign = cond (assign_op assign)?
// assign_op = '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^='
//           | '<<=' | '>>='
struct ast *parse_expr_assign(parser *p);
// expr = assign (',' expr)?
struct ast *parse_expr(parser *p);

struct ast *parse_type_name(parser *p);
struct ast *parse_func_args(parser *p);
struct ast *parse_expr_func_call(parser *p);

void parse(parser *p);

#endif
