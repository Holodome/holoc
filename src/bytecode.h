// Author: Holodome
// Date: 5.09.2021 
// File: pkby/src/bytecode.h
// Version: 0
//
//
#pragma once
#include "general.h"

#define MAXIMUM_RETURN_VALUES 16

enum {
    // General-purpose ops
    BYTECODE_NOP = 0x0,
    BYTECODE_STACK_PUSH = 0x1,
    BYTECODE_STACK_POP = 0x2,
    BYTECODE_GOTO = 0x3,
    BYTECODE_IFGOTO = 0x4,
    BYTECODE_CALL = 0x5,
    // 4 local 8-byte variables
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

// Bbytecode format:
// u16 number in block
// u8 opcode
// per-opcode data

#define PACK_4U8_TO_U32_(_a, _b, _c, _d) (((_a) << 0) | ((_b) << 8) | ((_c) << 16) | ((_d) << 24))
#define PACK_4U8_TO_U32(_a, _b, _c, _d) PACK_4U8_TO_U32_((u32)(_a), (u32)(_b), (u32)(_c), (u32)(_d))
#define BYTECODE_MAGIC_VALUE PACK_4U8_TO_U32('P', 'K', 'B', 'E')
#define BYTECODE_VERSION_MAJOR 0x0
#define BYTECODE_VERSION_MINOR 0x0
#define COMPILER_VERSION_MAJOR 0x0
#define COMPILER_VERSION_MINOR 0x0

typedef struct __attribute__((packed)) {
    // General header 32 bytes
    u32 magic_value;
    u8 version_major;
    u8 version_minor;
    u8 compiler_version_major;
    u8 compiler_version_minor;
    u64 compile_epoch;
    u8 __reserved[16];
} BytecodeExecutableHeader;