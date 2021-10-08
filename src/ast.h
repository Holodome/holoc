// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/ast.h
// Version: 0.1
//
// Description of abstract syntax trees
// 
// @NOTE in current implementation (13.09.21) AST uses single structure to repsent all differnt asts 
// using what is known as tagged union.
// This way all asts can be represented with structure of constant size, which is benefitial
// for operating in bulk
// However, actaul perfomance of tagged union vs inheritance structure is debatable,
// because ast in greater than cache line, and the program actually makes no operations on asts in bulk.
#pragma once
#include "common.h"

struct Out_Stream;

enum {
    AST_NONE,
    
    AST_BLOCK, // AST_Block
    AST_LIT, // AST_Lit
    AST_IDENT, // AST_Ident
    AST_TYPE, // AST_Type
    AST_UNARY, // AST_Unary
    AST_BINARY, // AST_Binary
    AST_ASSIGN, // AST_Assign
    AST_PRINT, // AST_Print
    AST_DECL, // AST_Decl
    AST_RETURN, // AST_Return 
    AST_IF, // AST_If
    AST_WHILE, // AST_While
    AST_FUNC_SIGNATURE, // AST_Func_Sign
    AST_FUNC_DECL, // AST_Func_Decl
    AST_FUNC_CALL, // AST_Func_Call
    
    AST_COUNT
};

typedef struct AST AST;

// @NOTE(hl): Currently the only data structure needed for use in asts is ordered list.
// Requirements for list:
// 1) Order
// 2) Fast addition of individual elements (implies linked list)
typedef struct AST_List {
    struct AST *sentinel;
    
    bool DBG_is_initialized;
    u32 DBG_len;
} AST_List;

// @NOTE(hl): Shorthand for writing iterative for loops
#define AST_LIST_ITER_(_sentinel, _it) for (AST *_it = (_sentinel)->next; _it != (_sentinel); _it = _it->next)
#define AST_LIST_ITER(_list, _it) AST_LIST_ITER_((_list)->sentinel, _it)
AST_List create_ast_list(AST *sentinel);
void ast_list_add(AST_List *list, AST *ast);

//
// Per-kind declarations
//

typedef struct {
    AST_List statements;
} AST_Block;

enum {
    AST_LIT_NONE,  
    AST_LIT_INT,  
    AST_LIT_REAL,
    AST_LIT_STR,
    AST_LIT_COUNT,
};

typedef struct {
    u32 kind;
    union {
        String_ID value_str;
        i64 value_int;
        f64 value_real;  
    };
} AST_Lit;

typedef struct {
    AST *lvalue;
    AST *rvalue;
} AST_Assign;

typedef struct {
    String_ID name;
} AST_Ident;

enum {
    AST_UNARY_NONE,
    AST_UNARY_MINUS, // -
    AST_UNARY_PLUS, // +
    AST_UNARY_LOGICAL_NOT, // ~
    AST_UNARY_NOT, // !
    AST_UNARY_COUNT,
};

const char *get_ast_unary_kind_str(u32 kind);
const char *get_ast_unary_kind_name_str(u32 kind);

typedef struct {
    u32 kind;
    AST *expr;
} AST_Unary;

enum {
    AST_BINARY_NONE,  
    AST_BINARY_ADD, // +
    AST_BINARY_SUB, // -
    AST_BINARY_MUL, // *
    AST_BINARY_DIV, // /
    AST_BINARY_MOD, // %
    AST_BINARY_LE, // <=
    AST_BINARY_L, // <
    AST_BINARY_GE, // >=
    AST_BINARY_G, // >
    AST_BINARY_EQ, // == 
    AST_BINARY_NEQ, // != 
    AST_BINARY_AND, // &
    AST_BINARY_OR, // |
    AST_BINARY_XOR, // ^
    AST_BINARY_LSHIFT, // <<
    AST_BINARY_RSHIFT, // >>
    AST_BINARY_LOGICAL_AND, // &&
    AST_BINARY_LOGICAL_OR, // ||
    AST_BINARY_COUNT,  
};

const char *get_ast_binary_kind_str(u32 kind);
const char *get_ast_binary_kind_name_str(u32 kind);

typedef struct {
    u32 kind;
    AST *left;
    AST *right;
} AST_Binary;

typedef struct {
    AST *ident;
    AST *expr;
    AST *type;
    bool is_immutable;
} AST_Decl;

typedef struct {
    AST_List arguments;
    AST_List return_types;
} AST_Func_Sign;

typedef struct {
    AST *name;
    AST *sign;
    AST *block;
} AST_Func_Decl;

typedef struct {
    AST_List vars;
} AST_Return;

typedef struct {
    AST *cond;
    AST *block;
    AST *else_block;
} AST_If;

typedef struct {
    AST *cond;
    AST *block;
} AST_While;

typedef struct {
    AST *callable;
    AST_List arguments;
} AST_Func_Call;

typedef struct {
    AST_List arguments;
} AST_Print;

enum {
    AST_TYPE_NONE = 0x0,
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_BOOL,
    AST_TYPE_STR,
    AST_TYPE_PROC,
    AST_TYPE_COUNT
};

typedef struct {
    u32 kind;
} AST_Type;

struct AST {
    u32 kind;
    Src_Loc src_loc;
    // for use in linked lists
    AST *next;
    AST *prev;
    union  {
        AST_Block block;
        AST_Lit lit;
        AST_Assign assign;
        AST_Ident ident;
        AST_Unary unary;
        AST_Binary binary;
        AST_Decl decl;
        AST_Func_Sign func_sign;
        AST_Return returns;
        AST_If ifs;
        AST_While whiles;
        AST_Func_Call func_call;
        AST_Type type;
        AST_Print prints;
        AST_Func_Decl func_decl;
    };
};

struct Compiler_Ctx;
void fmt_ast_tree(struct Compiler_Ctx *ctx, struct Out_Stream *stream, AST *ast, u32 depth);