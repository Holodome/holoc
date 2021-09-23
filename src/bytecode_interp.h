#pragma once 
#include "bytecode.h"
#include "stream.h"
#include "hashing.h"
#include "memory.h"
#include "ast.h"

typedef struct {
    u64 storage;
} BytecodeInterpVariable;

typedef struct BytecodeInterpStack {
    u64 num_variables;
    Hash64 variable_hash;
    BytecodeInterpVariable *variables;
    
    struct BytecodeInterpStack *parent;
} BytecodeInterpStack;

typedef struct {
    MemoryArena arena;
    InStream *in;
    
    BytecodeInterpStack *curr_stack;
    u64 var0;
    u64 var1;
    u64 var2;
    u64 var3;
} BytecodeInterp;

BytecodeInterp *create_bytecode_interp(InStream *in);