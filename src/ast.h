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
    AST_ASSIGN,
    AST_IDENT,
    AST_UNARY,
    AST_BINARY,
    AST_PRINT,
    
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

typedef struct AST {
    u32 kind;
    SourceLocation source_loc;
    union  {
        struct {
            // linked list of statements
            struct AST *first_statement;      
        } block;
        struct {
            u32 kind;
            union {
                i64 value_int;
                f64 value_real;  
            };
        } literal;
        struct {
            struct AST *ident;
            struct AST *expr;
        } assign;
        struct {
            char *name;
        } ident;
        struct {
            u32 kind;
            struct AST *expr;
        } unary;
        struct {
            u32 kind;
            struct AST *left;
            struct AST *right;
        } binary;
        struct {
            struct AST *expr;
        } print;
    };
    // for use in linked lists
    struct AST *next;
} AST;
