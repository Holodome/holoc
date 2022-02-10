#ifndef COND_INCL_AST
#define COND_INCL_AST

#include "general.h"

struct ast;
struct token;

// Generally, exprsession inside the #if are named constant, but in reality they
// implement only a subset of them (like there is no operator sizeof in #if's).
// So it has been decided to put all of these expression ast builders into
// separate functions.
// Expression parsing should be started from 'ternary' call because comma is not
// allowed in top-level expression context.
//
// primary = '(' expr ')'
//         | num
struct ast *ast_cond_incl_expr_primary(struct token **tokp);
// unary = '+' primary
//       | '-' primary
//       | '!' primary
//       | '~' primary
//       | primary
struct ast *ast_cond_incl_expr_unary(struct token **tokp);
// mul = unary ('*' unary |
//              '/' unary |
//              '%' unary)*
struct ast *ast_cond_incl_expr_mul(struct token **tokp);
// add = mul ('+' mul |
//            '-' mul)*
struct ast *ast_cond_incl_expr_add(struct token **tokp);
// shift = add ('<<' add |
//              '>>' add)*
struct ast *ast_cond_incl_expr_shift(struct token **tokp);
// rel = shift ('<=' shift |
//              '<'  shift |
//              '>'  shift |
//              '>=' shift)*
struct ast *ast_cond_incl_expr_rel(struct token **tokp);
// eq = rel ('!=' rel |
//           '==' rel)*
struct ast *ast_cond_incl_expr_eq(struct token **tokp);
// and = eq ('&' eq)*
struct ast *ast_cond_incl_expr_and(struct token **tokp);
// xor = and ('^' and)*
struct ast *ast_cond_incl_expr_xor(struct token **tokp);
// or = xor ('|' xor)*
struct ast *ast_cond_incl_expr_or(struct token **tokp);
// land = or ('&&' or)*
struct ast *ast_cond_incl_expr_land(struct token **tokp);
// lor = land ('||' land)*
struct ast *ast_cond_incl_expr_lor(struct token **tokp);
// ternary = lor ('?' expr ':' ternary)?
struct ast *ast_cond_incl_expr_ternary(struct token **tokp);
// expr = ternary (',' expr)?
struct ast *ast_cond_incl_expr(struct token **tokp);
// Returns intmax_t for constant expression.
int64_t ast_cond_incl_eval(struct ast *node);

#endif
