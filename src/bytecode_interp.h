// Author: Holodome
// Date: 23.09.2021 
// File: pkby/src/bytecode_interp.h
// Version: 0
#pragma once 
#include "bytecode.h"
#include "lib/stream.h"
#include "lib/hashing.h"
#include "lib/memory.h"
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
    Memory_Arena arena;
    In_Stream *in;
    
    BytecodeInterpStack *curr_stack;
    u64 var0;
    u64 var1;
    u64 var2;
    u64 var3;
} BytecodeInterp;

BytecodeInterp *create_bytecode_interp(In_Stream *in);