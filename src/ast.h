#ifndef AST_H
#define AST_H

#include "types.h"

struct c_type;
struct buffer_writer;
struct allocator;
struct token;

typedef enum {
    AST_NONE      = 0x0,   // Do nothing
    AST_ID        = 0x1,   // Identifier
    AST_STR       = 0x2,   // Sting literal
    AST_NUM       = 0x3,   // Number literal
    AST_UN        = 0x4,   // Unary -a
    AST_BIN       = 0x5,   // Binary a - b
    AST_TER       = 0x6,   // Ternary a ? b : c
    AST_IF        = 0x7,   // if (x) {}
    AST_FOR       = 0x8,   // for, while
    AST_DO        = 0x9,   // do { } while
    AST_SWITCH    = 0xA,   // switch (x) {}
    AST_CASE      = 0xB,   // case x: {}
    AST_BLOCK     = 0xC,   // {}
    AST_GOTO      = 0xD,   // goto label;
    AST_LABEL     = 0xE,   // label:
    AST_FUNC_CALL = 0xF,   // memcpy(...)
    AST_CAST      = 0x10,  // (int) x
    AST_MEMB      = 0x11,  // a.x
    AST_RETURN    = 0x12,  // return;

    AST_STRUCT     = 0x13,  // struct x { int x; };
    AST_ENUM       = 0x14,  // enum x { A, B, C };
    AST_ENUM_VALUE = 0x15,
    AST_DECL       = 0x16,  // int x;
    AST_FUNC       = 0x17,
    AST_TYPEDEF    = 0x18,
    AST_TYPE       = 0x19,
} ast_kind;

typedef enum {
    AST_UN_MINUS = 0x1,  // -
    AST_UN_PLUS  = 0x2,  // +
    AST_UN_LNOT  = 0x3,  // !
    AST_UN_NOT   = 0x4,  // ~

    AST_UN_PREINC  = 0x5,  // ++x
    AST_UN_POSTINC = 0x6,  // x++
    AST_UN_PREDEC  = 0x7,  // --x
    AST_UN_POSTDEC = 0x8,  // x--

    AST_UN_DEREF = 0x9,  // *
    AST_UN_ADDR  = 0xA,  // &
} ast_unary_kind;

typedef enum {
    AST_BIN_ADD    = 0x1,   // +
    AST_BIN_SUB    = 0x2,   // -
    AST_BIN_MUL    = 0x3,   // *
    AST_BIN_DIV    = 0x4,   // /
    AST_BIN_MOD    = 0x5,   // %
    AST_BIN_LE     = 0x6,   // <=
    AST_BIN_L      = 0x7,   // <
    AST_BIN_GE     = 0x8,   // >=
    AST_BIN_G      = 0x9,   // >
    AST_BIN_EQ     = 0xA,   // ==
    AST_BIN_NEQ    = 0xB,   // !=
    AST_BIN_AND    = 0xC,   // &
    AST_BIN_OR     = 0xD,   // |
    AST_BIN_XOR    = 0xE,   // ^
    AST_BIN_LSHIFT = 0xF,   // <<
    AST_BIN_RSHIFT = 0x10,  // >>
    AST_BIN_LAND   = 0x11,  // &&
    AST_BIN_LOR    = 0x12,  // ||

    AST_BIN_A       = 0x13,  // =
    AST_BIN_ADDA    = 0x14,  // +=
    AST_BIN_SUBA    = 0x16,  // -=
    AST_BIN_MULA    = 0x17,  // *=
    AST_BIN_DIVA    = 0x18,  // /=
    AST_BIN_MODA    = 0x19,  // %=
    AST_BIN_LSHIFTA = 0x1A,  // <<=
    AST_BIN_RSHIFTA = 0x1B,  // >>=
    AST_BIN_ANDA    = 0x1C,  // &=
    AST_BIN_ORA     = 0x1D,  // |=
    AST_BIN_XORA    = 0x1E,  // ^=

    AST_BIN_COMMA = 0x1F,  // ,
} ast_binary_kind;

#define _AST_FIELDS   \
    struct ast *next; \
    ast_kind kind;

typedef struct ast {
    _AST_FIELDS;
} ast;

typedef struct ast_identifier {
    _AST_FIELDS;
    string ident;
} ast_identifier;

typedef struct ast_string {
    _AST_FIELDS;
    string str;
    struct c_type *type;
} ast_string;

typedef struct ast_number {
    _AST_FIELDS;
    uint64_t uint_value;
    long double float_value;
    struct c_type *type;
} ast_number;

typedef struct ast_unary {
    _AST_FIELDS;
    ast_unary_kind un_kind;
    ast *expr;
} ast_unary;

typedef struct ast_binary {
    _AST_FIELDS;
    ast_binary_kind bin_kind;
    ast *left;
    ast *right;
} ast_binary;

typedef struct ast_ternary {
    _AST_FIELDS;
    ast *cond;
    ast *cond_true;
    ast *cond_false;
} ast_ternary;

typedef struct ast_if {
    _AST_FIELDS;
    ast *cond;
    ast *cond_true;
    ast *cond_false;
} ast_if;

typedef struct ast_for {
    _AST_FIELDS;
    ast *init;
    ast *cond;
    ast *iter;
    ast *loop;
} ast_for;

typedef struct ast_do {
    _AST_FIELDS;
    ast *loop;
    ast *cond;
} ast_do;

typedef struct ast_switch {
    _AST_FIELDS;
    ast *expr;
    ast *sts;
} ast_switch;

typedef struct ast_case {
    _AST_FIELDS;
    ast *expr;
    ast *sts;
} ast_case;

typedef struct ast_block {
    _AST_FIELDS;
    ast *st;
} ast_block;

typedef struct ast_goto {
    _AST_FIELDS;
} ast_goto;

typedef struct ast_label {
    _AST_FIELDS;
    string label;
} ast_label;

typedef struct ast_func_call {
    _AST_FIELDS;
} ast_func_call;

typedef struct ast_cast {
    _AST_FIELDS;
    struct c_type *type;
    ast *expr;
} ast_cast;

typedef struct ast_member {
    _AST_FIELDS;
    ast *obj;
    string field;
} ast_member;

typedef struct ast_return {
    _AST_FIELDS;
    ast *expr;
} ast_return;

typedef struct ast_struct_decl {
    _AST_FIELDS;
} ast_struct_decl;

typedef struct ast_enum_field {
    _AST_FIELDS;
    string name;
    uint64_t value;
} ast_enum_field;

typedef struct ast_enum_decl {
    _AST_FIELDS;
    string name;
    ast_enum_field *fields;
} ast_enum_decl;

typedef struct ast_decl {
    _AST_FIELDS;
} ast_decl;

typedef struct ast_func {
    _AST_FIELDS;
} ast_func;

typedef struct ast_typedef {
    _AST_FIELDS;
    struct ast *underlying;
    string name;
} ast_typedef;

typedef struct ast_type {
    _AST_FIELDS;
    struct c_type *type;
} ast_type;

void fmt_astw(void *ast, struct buffer_writer *w);
uint32_t fmt_ast(void *ast, char *buf, uint32_t buf_size);
void fmt_ast_verbosew(void *ast, struct buffer_writer *w);
uint32_t fmt_ast_verbose(void *ast, char *buf, uint32_t buf_size);

// Creates ast of given kind. Allocates memory needed for structure of that kind
// and sets kind.
void *make_ast(struct allocator *a, ast_kind kind);
//
// Generally, exprsession inside the #if are named constant, but in reality they
// implement only a subset of them (like there is no operator sizeof in #if's).
// So it has been decided to put all of these expression ast builders into
// separate functions.
//
// primary = '(' expr ')'
//         | num
ast *ast_cond_incl_expr_primary(struct allocator *a, struct token **tokp);
// unary = '+' primary
//       | '-' primary
//       | '!' primary
//       | '~' primary
//       | primary
ast *ast_cond_incl_expr_unary(struct allocator *a, struct token **tokp);
// mul = unary ('*' unary |
//              '/' unary |
//              '%' unary)*
ast *ast_cond_incl_expr_mul(struct allocator *a, struct token **tokp);
// add = mul ('+' mul |
//            '-' mul)*
ast *ast_cond_incl_expr_add(struct allocator *a, struct token **tokp);
// shift = add ('<<' add |
//              '>>' add)*
ast *ast_cond_incl_expr_shift(struct allocator *a, struct token **tokp);
// rel = shift ('<=' shift |
//              '<'  shift |
//              '>'  shift |
//              '>=' shift)*
ast *ast_cond_incl_expr_rel(struct allocator *a, struct token **tokp);
// eq = rel ('!=' rel |
//           '==' rel)*
ast *ast_cond_incl_expr_eq(struct allocator *a, struct token **tokp);
// and = eq ('&' eq)*
ast *ast_cond_incl_expr_and(struct allocator *a, struct token **tokp);
// xor = and ('^' and)*
ast *ast_cond_incl_expr_xor(struct allocator *a, struct token **tokp);
// or = xor ('|' xor)*
ast *ast_cond_incl_expr_or(struct allocator *a, struct token **tokp);
// land = or ('&&' or)*
ast *ast_cond_incl_expr_land(struct allocator *a, struct token **tokp);
// lor = land ('||' land)*
ast *ast_cond_incl_expr_lor(struct allocator *a, struct token **tokp);
// ternary = lor ('?' expr ':' ternary)?
ast *ast_cond_incl_expr_ternary(struct allocator *a, struct token **tokp);
// expr = ternary (',' expr)?
ast *ast_cond_incl_expr(struct allocator *a, struct token **tokp);

#endif
