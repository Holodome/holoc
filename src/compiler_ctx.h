/*
Author: Holodome
Date: 14.10.2021
File: src/compiler_ctx.h
Version: 0
*/
#ifndef COMPILER_CTX_H
#define COMPILER_CTX_H

#include "lib/general.h"

struct Memory_Arena;
struct Error_Reporter;
struct File_Registry;
struct String_Storage;

// Storage structure that has lifetime of the whole program.
typedef struct Compiler_Ctx {
    struct Memory_Arena *arena;
    struct Error_Reporter *er;
    struct File_Registry *fr;
    struct String_Storage *ss;
} Compiler_Ctx;

Compiler_Ctx *create_compiler_ctx(void);

#endif