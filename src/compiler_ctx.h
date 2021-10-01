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

typedef struct CompilerCtx {
    MemoryArena arena;
    ErrorReporter *er;
    StringStorage *ss;
    SymbolTable *st;
} CompilerCtx;

CompilerCtx *create_compiler_ctx(void);
void destroy_compiler_ctx(CompilerCtx *ctx);

