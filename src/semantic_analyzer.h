/* 
Author: Holodome
Date: 01.10.2021 
File: pkby/src/semantic_analyzer.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "compiler_ctx.h"
#include "symbol_table.h"

void do_semantic_analysis(CompilerCtx *ctx, AST *toplevel);