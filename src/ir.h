/*
Author: Holodome
Date: 03.10.2021
File: src/ir.h
Version: 0

Intermidate representation
*/
#pragma once 
#include "lib/general.h"
#include "platform/memory.h"

#include "compiler_ctx.h"
#include "ast.h"

enum {
    IR_OP_NONE = 0x0,
    IR_OP_ALLOCA = 0x1,
    IR_OP_CALL = 0x2,  
};

typedef struct IRNode {
    
    
    
    struct IRNode *next;
} IRNode;

typedef struct {
    Compiler_Ctx *ctx;
    Memory_Arena *arena;

} IR;

IR *create_ir(Compiler_Ctx *ctx, Memory_Arena *arena);
void ir_process_toplevel(IR *ir, AST *toplevel);