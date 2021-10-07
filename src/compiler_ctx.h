/* 
Author: Holodome
Date: 01.10.2021 
File: pkby/src/compiler_ctx.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "error_reporter.h"
#include "string_storage.h"
#include "symbol_table.h"

typedef struct Compiler_Ctx {
    Memory_Arena arena;
    Error_Reporter *er;
    String_Storage *ss;
    Symbol_Table *st;
} Compiler_Ctx;

Compiler_Ctx *create_compiler_ctx(void);
void destroy_compiler_ctx(Compiler_Ctx *ctx);

