// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/ast.h
// Version: 0
// ast.h
//
// Description of abstract syntax trees
// 
// @NOTE in current implementation (13.09.21) AST uses single structure to repsent all differnt asts 
// using what is known as tagged union.
// However, in provides little to none benefits in case of asts, so switching to 'inheritance' model
// could be made.
// In ISO C17, there is still no way to accomplish this.
// However, ms-extensions can be used
#pragma once
#include "general.h"

#include "strings.h"
#include "stream.h"
#include "error_reporter.h"

enum {
    AST_NONE,
    
    AST_BLOCK,
    AST_LITERAL,
    AST_IDENT,
    AST_TYPE,

    AST_UNARY,
    AST_BINARY,
    
    AST_ASSIGN,
    AST_PRINT,
    AST_DECL,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    
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

enum {
    AST_TYPE_NONE = 0x0,
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_COUNT
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

#define AST_LIST_ITER(_list, _it) for (AST *_it = (_list)->first; _it; _it = _it->next) 
void ast_list_add(ASTList *list, AST *ast);

struct AST {
    u32 kind;
    SrcLoc src_loc;
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
            AST *ident;
            AST *expr;
            
            u32 type;
            b32 is_immutable;
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
            AST *cond;
            AST *block;
        } while_st;
        struct {
            AST *callable;
            ASTList arguments;
        } func_call;
        struct {
            ASTList arguments;
        } print_st;
        struct {
            u32 kind;
        } type;
    };
};

// Prints ast tree as text tree-like structure.
void fmt_ast_tree(OutStream *stream, AST *ast, u32 depth);
// Prints ast expression as code, trying to reverse get the input
// Used in debugging expression parsing 
// @NOTE Can be modified to format whole ast tree as source code
void fmt_ast_expr(OutStream *stream, AST *ast);