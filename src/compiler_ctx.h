/* 
Author: Holodome
Date: 01.10.2021 
File: pkby/src/compiler_ctx.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

typedef struct Compiler_Ctx {
    struct Memory_Arena *arena;
    struct Error_Reporter *er;
    struct String_Storage *ss;
    struct Symbol_Table *st;
} Compiler_Ctx;

Compiler_Ctx *create_compiler_ctx(void);
void destroy_compiler_ctx(Compiler_Ctx *ctx);

