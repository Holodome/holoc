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
#include "lib/general.h"

#include "lib/strings.h"
#include "lib/stream.h"
#include "error_reporter.h"
#include "string_storage.h"

enum {
    AST_NONE,
    
    AST_BLOCK, // ASTBlock
    AST_LIT, // ASTLit
    AST_IDENT, // ASTIdent
    AST_TYPE, // ASTType
    AST_UNARY, // ASTUnary
    AST_BINARY, // ASTBinary
    AST_ASSIGN, // ASTAssign
    AST_PRINT, // ASTPrint
    AST_DECL, // ASTDecl
    AST_RETURN, // ASTReturn 
    AST_IF, // ASTIf
    AST_WHILE, // ASTWhile
    AST_FUNC_SIGNATURE, // ASTFuncSign
    AST_FUNC_DECL, // ASTFuncDecl
    AST_FUNC_CALL, // ASTFuncCall
    
    AST_COUNT
};

typedef struct AST AST;

// @NOTE(hl): Currently the only data structure needed for use in asts is ordered list.
// Requirements for list:
// 1) Order
// 2) Fast addition of individual elements (implies linked list)
typedef struct ASTList {
    struct AST *sentinel;
    
    bool DBG_is_initialized;
    u32 DBG_len;
} ASTList;

// @NOTE(hl): Shorthand for writing iterative for loops
#define AST_LIST_ITER_(_sentinel, _it) for (AST *_it = (_sentinel)->next; _it != (_sentinel); _it = _it->next)
#define AST_LIST_ITER(_list, _it) AST_LIST_ITER_((_list)->sentinel, _it)
ASTList create_ast_list(AST *sentinel);
void ast_list_add(ASTList *list, AST *ast);

//
// Per-kind declarations
//

typedef struct {
    ASTList statements;
} ASTBlock;

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
        StringID value_str;
        i64 value_int;
        f64 value_real;  
    };
} ASTLit;

typedef struct {
    AST *lvalue;
    AST *rvalue;
} ASTAssign;

typedef struct {
    StringID name;
} ASTIdent;

enum {
    AST_UNARY_NONE,
    AST_UNARY_MINUS, // -
    AST_UNARY_PLUS, // +
    AST_UNARY_LOGICAL_NOT, // ~
    AST_UNARY_NOT, // !
    AST_UNARY_COUNT,
};

typedef struct {
    u32 kind;
    AST *expr;
} ASTUnary;

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

typedef struct {
    u32 kind;
    AST *left;
    AST *right;
} ASTBinary;

typedef struct {
    AST *ident;
    AST *expr;
    AST *type;
    bool is_immutable;
} ASTDecl;

typedef struct {
    ASTList arguments;
    ASTList return_types;
} ASTFuncSign;

typedef struct {
    AST *name;
    AST *sign;
    AST *block;
} ASTFuncDecl;

typedef struct {
    ASTList vars;
} ASTReturn;

typedef struct {
    AST *cond;
    AST *block;
    AST *else_block;
} ASTIf;

typedef struct {
    AST *cond;
    AST *block;
} ASTWhile;

typedef struct {
    AST *callable;
    ASTList arguments;
} ASTFuncCall;

typedef struct {
    ASTList arguments;
} ASTPrint;

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
} ASTType;

struct AST {
    u32 kind;
    SrcLoc src_loc;
    // for use in linked lists
    AST *next;
    AST *prev;
    union  {
        ASTBlock block;
        ASTLit lit;
        ASTAssign assign;
        ASTIdent ident;
        ASTUnary unary;
        ASTBinary binary;
        ASTDecl decl;
        ASTFuncSign func_sign;
        ASTReturn returns;
        ASTIf ifs;
        ASTWhile whiles;
        ASTFuncCall func_call;
        ASTType type;
        ASTPrint prints;
        ASTFuncDecl func_decl;
    };
};

struct CompilerCtx;
void fmt_ast_tree(struct CompilerCtx *ctx, OutStream *stream, AST *ast, u32 depth);