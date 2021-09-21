#pragma once
#include "general.h"
#include "stream.h"
#include "hashing.h"
#include "memory.h"

typedef u8 bytecode_op;
enum {
    // General-purpose ops
    BYTECODE_NOP = 0x0,
    BYTECODE_STACK_PUSH = 0x1,
    BYTECODE_STACK_POP = 0x2,
    BYTECODE_GOTO = 0x3,
    BYTECODE_IFGOTO = 0x4,
    BYTECODE_CALL = 0x5,
    // 4 local 8-bit variables
    BYTECODE_LOAD0 = 0x10,
    BYTECODE_LOAD1 = 0x11,
    BYTECODE_LOAD2 = 0x12,
    BYTECODE_LOAD3 = 0x13,
    BYTECODE_STORE0 = 0x14,
    BYTECODE_STORE1 = 0x15,
    BYTECODE_STORE2 = 0x16,
    BYTECODE_STORE3 = 0x17,
    BYTECODE_LOADCONST0 = 0x18,
    BYTECODE_LOADCONST1 = 0x19,
    BYTECODE_LOADCONST2 = 0x1A,
    BYTECODE_LOADCONST3 = 0x1B,
    // Integer operations
    BYTECODE_ADD = 0x20,
    BYTECODE_SUB = 0x21,
    BYTECODE_DIV = 0x22,
    BYTECODE_MUL = 0x23,
    BYTECODE_MOD = 0x24,
    BYTECODE_AND = 0x25,
    BYTECODE_OR = 0x26,
    BYTECODE_XOR = 0x27,
    BYTECODE_LSFHIT = 0x28,
    BYTECODE_RSHIFT = 0x29,
    // Boolean operations
    BYTECODE_CMP = 0x30,
    BYTECODE_LOGICAL_AND = 0x31,
    BYTECODE_LOGICAL_OR = 0x32,
    // Floating-point operations
    BYTECODE_FLT_ADD = 0x40,
    BYTECODE_FLT_SUB = 0x41,
    BYTECODE_FLT_MUL = 0x42,
    BYTECODE_FLT_DIV = 0x43,
    
    BYTECODE_FLT_CMP = 0x50,
};

typedef struct {
    OutStream *out;
} BytecodeBuilder;

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

void init_bytecode_interp(BytecodeInterp *interp, InStream *in);