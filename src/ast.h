//
// ast.h
//
// description of abstract syntax trees. Parser turns tokenized input into AST, that is later executed
#pragma once
#include "general.h"

#include "filesystem.h"
#include "strings.h"

enum {
    AST_NONE,
    
    AST_BLOCK,
    AST_LITERAL,
    AST_IDENT,

    // Expressions:
    AST_UNARY,
    AST_BINARY,
    // Statements:    
    AST_ASSIGN,
    AST_PRINT,
    AST_DECL,
    AST_RETURN,
    AST_IF,
    
    AST_FUNC_SIGNATURE,
    AST_FUNC_DECL,
    AST_FUNC_CALL,
    
    AST_COUNT
};

enum {
    AST_LITERAL_NONE,  
    AST_LITERAL_INT,  
    AST_LITERAL_REAL,
    AST_LITERAL_STRING,
    AST_LITERAL_COUNT,
};

enum {
    AST_UNARY_NONE,
    AST_UNARY_MINUS, // -
    AST_UNARY_PLUS, // +
    AST_UNARY_LOGICAL_NOT, // ~
    AST_UNARY_NOT, // !
    AST_UNARY_COUNT,
};

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

typedef struct AST AST;

// Because in C there is no templates, lists types have to be chosen carefully.
// Basically, all is needed from list is fast iteration.
// Fastest iteration is possile with arrays, however use of dynamic arrays can lead to fragmentation
// And unecessary allocations
// So during construction, linked lists are used.
// @TODO it is possible to have ASTArray structure that is actually stored in AST, which is formed from the ASTList after its creation is finished
//  But this is sort of premature optimization because there are only so many times we actully iterate each of these lists
// (ofthen only one time)
typedef struct ASTList {
    AST *first;
    AST *last;
} ASTList;

void ast_list_add(ASTList *list, AST *ast);

struct AST {
    u32 kind;
    SourceLocation source_loc;
    // for use in linked lists
    AST *next;
    union  {
        struct {
            ASTList statements;
        } block;
        struct {
            u32 kind;
            union {
                i64 value_int;
                f64 value_real;  
            };
        } literal;
        struct {
            AST *ident;
            AST *expr;
        } assign;
        struct {
            char *name;
        } ident;
        struct {
            u32 kind;
            AST *expr;
        } unary;
        struct {
            u32 kind;
            AST *left;
            AST *right;
        } binary;
        struct {
            AST *expr;
        } print;
        struct {
            AST *ident;
            AST *expr;
        } decl;
        struct {
            ASTList arguments;
            ASTList return_types;
        } func_sign;
        struct {
            AST *name;
            AST *sign;
            AST *block;
        } func_decl;
        struct {
            ASTList vars;
        } return_st;
        struct {
            AST *cond;
            AST *block;
            AST *else_block;
        } if_st;
        struct {
            AST *callable;
            ASTList arguments;
        } func_call;
    };
};

void fmt_ast_tree_recursive(FmtBuffer *buf, AST *ast, u32 depth);