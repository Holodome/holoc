#pragma once 
#include "bytecode.h"
#include "memory.h"
#include "ast.h"
#include "error_reporter.h"

typedef struct BytecodeBuilderVar {
    const char *ident;
    u64 storage; // can be zero
    u32 type;
    b32 is_immutable;
    struct BytecodeBuilderVar *next;
} BytecodeBuilderVar;

typedef struct {
    MemoryArena arena;
    ErrorReporter *error_reporter;
    
    BytecodeBuilderVar *static_vars;
} BytecodeBuilder;

BytecodeBuilder *create_bytecode_builder(ErrorReporter *reporter);
void destroy_bytecode_builder(BytecodeBuilder *builder); 
void bytecode_builder_proccess_toplevel(BytecodeBuilder *builder, AST *toplevel);
void bytecode_builder_emit_code(BytecodeBuilder *builder, FileHandle *out);
