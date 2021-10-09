/*
Author: Holodome
Date: 03.10.2021
File: src/ir.h
Version: 0

Intermidate representation is stage between the parsing and compiling.
In this stage all information contained in AST trees should be turned into 
some intermediate code format that can be later optimized and compiled into 
actual code.
*/
#pragma once 
#include "common.h"
#include "lib/hashing.h"

struct Compiler_Ctx;
struct AST;

enum {
    IR_NODE_NONE  = 0x0,
    IR_NODE_LABEL = 0x1,
    IR_NODE_UN    = 0x2,
    IR_NODE_BIN   = 0x3,
    IR_NODE_VAR   = 0x4,
    IR_NODE_LIT   = 0x5,
    IR_NODE_IFGOTO = 0x6,
    IR_NODE_GOTO = 0x7,
    IR_NODE_IF_NOT_GOTO,
};

typedef struct {
    u32 number;
    String_ID id;
    bool is_temp;
} IR_Var;

typedef struct IR_Node {
    bool DBG_is_added;
    
    u32 kind;
    struct IR_Node *next;
    struct IR_Node *prev;
    union {
        struct {
            u32 index;
        } label;
        struct {
            u32 kind; 
            IR_Var what;
            IR_Var dest;
        } un;
        struct {
            u32 kind;
            IR_Var left;
            IR_Var right;
            IR_Var dest;
        } bin;
        struct {
            IR_Var dest;
            u32 kind;
            union {
                i64 int_value;
                f64 real_value;  
            };
        } lit;
        struct {
            IR_Var dest;
            IR_Var source;
        } var;
        struct {
            IR_Var expr;
            u32 label_idx;
        } if_goto;
        struct {
            u32 label_idx;
        } gotos;
        struct {
            IR_Var expr;
            u32 label_idx;
        } if_not_goto;
    };
} IR_Node;

typedef struct {
    // IR_Node *first;
    // IR_Node *last;
    IR_Node sentinel;
} IR_Node_List;

void add_node(IR_Node_List *list, IR_Node *node);

typedef struct {
    String_ID name;
    u32 nargs;
    u32 *argument_types;
    String_ID *argument_names;
    u32 nreturn_values;
    u32 *return_value_types;
    
    IR_Node_List node_list;
} IR_Func;

#define MAX_VARIBALES 1024

typedef struct {
    struct Compiler_Ctx *ctx;
    struct Memory_Arena *arena;
    // Current compilation unit context
    u32 next_label_idx;
    u32 current_temp_n;
    u32 nvars;
    IR_Var vars[MAX_VARIBALES];
    Hash64 variable_hash;
    IR_Node_List *node_list;
} IR;

IR *create_ir(struct Compiler_Ctx *ctx, Memory_Arena *arena);
void ir_process_toplevel(IR *ir, struct AST *toplevel);