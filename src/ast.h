#ifndef C_AST_H
#define C_AST_H

#include "types.h"

struct c_type;
struct buffer_writer;

typedef enum {
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
} ast_kind;

typedef enum {
    AST_UNARY_MINUS = 0x1,  // -
    AST_UNARY_PLUS  = 0x2,  // +
    AST_UNARY_LNOT  = 0x3,  // !
    AST_UNARY_NOT   = 0x4,  // ~

    AST_UNARY_PREINC  = 0x5,  // ++x
    AST_UNARY_POSTINC = 0x6,  // x++
    AST_UNARY_PREDEC  = 0x7,  // --x
    AST_UNARY_POSTDEC = 0x8,  // x--

    AST_UNARY_DEREF = 0x9,  // *
    AST_UNARY_ADDR  = 0xA,  // &
} ast_unary_kind;

typedef enum {
    AST_BINARY_ADD    = 0x1,   // +
    AST_BINARY_SUB    = 0x2,   // -
    AST_BINARY_MUL    = 0x3,   // *
    AST_BINARY_DIV    = 0x4,   // /
    AST_BINARY_MOD    = 0x5,   // %
    AST_BINARY_LE     = 0x6,   // <=
    AST_BINARY_L      = 0x7,   // <
    AST_BINARY_GE     = 0x8,   // >=
    AST_BINARY_G      = 0x9,   // >
    AST_BINARY_EQ     = 0xA,   // ==
    AST_BINARY_NEQ    = 0xB,   // !=
    AST_BINARY_AND    = 0xC,   // &
    AST_BINARY_OR     = 0xD,   // |
    AST_BINARY_XOR    = 0xE,   // ^
    AST_BINARY_LSHIFT = 0xF,   // <<
    AST_BINARY_RSHIFT = 0x10,  // >>
    AST_BINARY_LAND   = 0x11,  // &&
    AST_BINARY_LOR    = 0x12,  // ||

    AST_BINARY_A       = 0x13,  // =
    AST_BINARY_ADDA    = 0x14,  // +=
    AST_BINARY_SUBA    = 0x16,  // -=
    AST_BINARY_MULA    = 0x17,  // *=
    AST_BINARY_DIVA    = 0x18,  // /=
    AST_BINARY_MODA    = 0x19,  // %=
    AST_BINARY_LSHIFTA = 0x1A,  // <<=
    AST_BINARY_RSHIFTA = 0x1B,  // >>=
    AST_BINARY_ANDA    = 0x1C,  // &=
    AST_BINARY_ORA     = 0x1D,  // |=
    AST_BINARY_XORA    = 0x1E,  // ^=

    AST_BINARY_COMMA = 0x1F,  // ,
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
    ast *left;
    ast *right;
} ast_ternary;

typedef struct ast_if {
    _AST_FIELDS;
    ast *cond;
    ast *expr;
    ast *else_expr;
} ast_if;

typedef struct ast_for {
    _AST_FIELDS;
    ast *init;
    ast *cond;
    ast *inc;
} ast_for;

typedef struct ast_do {
    _AST_FIELDS;
    ast *expr;
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
    ast *sts;
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

void fmt_astw(void *ast, struct buffer_writer *w);
uint32_t fmt_ast(ast *ast, char *buf, uint32_t buf_size);

#endif
