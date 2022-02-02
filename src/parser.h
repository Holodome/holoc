#ifndef PARSER_H
#define PARSER_H

#include "types.h"

struct allocator;
struct token_iter;
struct ast;

typedef struct parser {
    struct allocator *a;

    struct token_iter *it;
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
