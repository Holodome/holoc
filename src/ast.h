//
// ast.h
//
// description of abstract syntax trees. Parser turns tokenized input into AST, that is later executed
#pragma once
#include "general.h"

#include "filesystem.h"

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
    
    AST_FUNC_SIGNATURE,
    AST_FUNC_DECL,
    
    AST_COUNT
};

enum {
    AST_LITERAL_NONE,  
    AST_LITERAL_INT,  
    AST_LITERAL_REAL,
    AST_LITERAL_COUNT,
};

enum {
    AST_UNARY_NONE,
    AST_UNARY_MINUS,
    AST_UNARY_PLUS,
    AST_UNARY_COUNT,
};

enum {
    AST_BINARY_NONE,  
    AST_BINARY_ADD,  
    AST_BINARY_SUB,  
    AST_BINARY_MUL,  
    AST_BINARY_DIV,  
    AST_BINARY_COUNT,  
};

typedef struct AST AST;
struct AST {
    u32 kind;
    SourceLocation source_loc;
    union  {
        struct {
            // linked list of statements
            AST *first_statement;      
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
            AST *arguments;
            AST *outs;
        } func_sign;
        struct {
            AST *name;
            AST *sign;
            AST *block;
        } func_decl;
    };
    // for use in linked lists
    AST *next;
};
