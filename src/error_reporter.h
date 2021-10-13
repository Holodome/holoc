/*
Author: Holodome
Date: 12.10.2021
File: src/error_reporter.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "file_registry.h" // Src_Loc

struct Token;
struct AST;

typedef struct Error_Reporter {
    u32 warning_count;
    u32 error_count;
} Error_Reporter;

void report_errorv(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, va_list args);
void report_error(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, ...);
void report_error_tok(Error_Reporter *reporter, struct Token *token, const char *msg, ...);
void report_error_ast(Error_Reporter *reporter, struct AST *ast, const char *msg, ...);
bool is_error_reported(Error_Reporter *reporter);