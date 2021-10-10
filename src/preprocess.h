/*
Author: Holodome
Date: 10.10.2021
File: src/preprocess.h
Version: 0
*/
#pragma once 
#include "common.h"

struct Compiler_Ctx;
struct Memory_Arena;

enum {
    PREPROCESSOR_DIRECTIVE_NONE     = 0x0, // # - null directive
    PREPROCESSOR_DIRECTIVE_DEFINE   = 0x1, 
    PREPROCESSOR_DIRECTIVE_UNDEF    = 0x2, 
    PREPROCESSOR_DIRECTIVE_INCLUDE  = 0x3, 
    PREPROCESSOR_DIRECTIVE_IF       = 0x4, 
    PREPROCESSOR_DIRECTIVE_IFDEF    = 0x5, 
    PREPROCESSOR_DIRECTIVE_IFNDEF   = 0x6, 
    PREPROCESSOR_DIRECTIVE_ELSE     = 0x7, 
    PREPROCESSOR_DIRECTIVE_ELIFDEF  = 0x8, // c2x
    PREPROCESSOR_DIRECTIVE_ELIFNDEF = 0x9, // c2x
    PREPROCESSOR_DIRECTIVE_PRAGMA   = 0xA, 
    PREPROCESSOR_DIRECTIVE_COUNT
};

typedef struct Preprocessor_Macro_Argument {
    String_ID name;
    struct Preprocessor_Macro_Argument *next;
} Preprocessor_Macro_Argument;

typedef struct Preprocessor_Macro_Definition_Token {
    struct Token *token;
    struct Preprocessor_Macro_Definition_Token *next;
} Preprocessor_Macro_Definition_Token;

typedef struct Preprocessor_Macro {
    String_ID name;
    bool is_function_like; // MACRO or MACRO()
    Preprocessor_Macro_Argument *first_argument;
    u32 argument_count;
    Preprocessor_Macro_Definition_Token *definition;
    
    struct Preprocessor_Macro *next;
    struct Preprocessor_Macro *prev;
} Preprocessor_Macro;

typedef struct Preprocessor_Ctx {
    struct Compiler_Ctx *ctx;
    struct Memory_Arena *arena;
    
    String_ID directive_strings[PREPROCESSOR_DIRECTIVE_COUNT];
    Preprocessor_Macro macro_sentinel;
} Preprocessor_Ctx;

Preprocessor_Ctx *create_preprocessor(struct Compiler_Ctx *ctx);
void register_preprocessor_strings(Preprocessor_Ctx *pp);