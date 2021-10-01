// Author: Holodome
// Date: 23.08.2021 
// File: pkby/src/bytecode_builder.h
// Version: 0
#pragma once 
#include "bytecode.h"
#include "platform/memory.h"
#include "ast.h"
#include "compiler_ctx.h"

#define BYTECODE_BUILDER_CODE_PAGE_SIZE KB(4)

typedef struct BytecodeBuilderVar {
    StringID name;
    u64 storage; // can be zero
    u32 type;
    struct BytecodeBuilderVar *next;
} BytecodeBuilderVar;

typedef struct BytecodeBuilderCodePage {
    u8 buffer[BYTECODE_BUILDER_CODE_PAGE_SIZE];
    u16 buffer_used;
    
    struct BytecodeBuilderCodePage *next;
} BytecodeBuilderCodePage;

typedef struct BytecodeBuilderFunction {
    u64 name_hash;
    BytecodeBuilderVar *first_argument;
    BytecodeBuilderCodePage *first_code_page;
    u32 nreturn_values;
    u32 return_values[MAXIMUM_RETURN_VALUES];
    struct BytecodeBuilderFunction *next;
} BytecodeBuilderFunction;

typedef struct BytecodeBuilderBlockInfo {
    BytecodeBuilderVar *variables;
    
    struct BytecodeBuilderBlockInfo *parent;
} BytecodeBuilderBlockInfo;

typedef struct {
    MemoryArena arena;
    CompilerCtx *ctx;
    
    BytecodeBuilderVar *static_vars;
    BytecodeBuilderFunction *functions;
    
    BytecodeBuilderBlockInfo *current_block_info;
    
    BytecodeBuilderVar *var_free_list;
    BytecodeBuilderBlockInfo *block_info_free_list;
} BytecodeBuilder;

BytecodeBuilder *create_bytecode_builder(CompilerCtx *ctx);
void destroy_bytecode_builder(BytecodeBuilder *builder); 
void bytecode_builder_proccess_toplevel(BytecodeBuilder *builder, AST *toplevel);
void bytecode_builder_emit_code(BytecodeBuilder *builder, OSFileHandle *out);
