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
struct Type_Table;

// Storage structure that has lifetime of the whole program.
// This is associated with a single compilation unit
typedef struct Compiler_Ctx {
    struct Memory_Arena *arena;
    struct Error_Reporter *er;
    // @TODO(hl): This could be program part??
    struct File_Registry *fr;
    struct String_Storage *ss;
    // @NOTE(hl): This is placed here and not a part of some compiler stage because
    //  it is referenced during all compilation stages, but most importantly, in preprocessor
    struct Type_Table *tt;
} Compiler_Ctx;

Compiler_Ctx *create_compiler_ctx(void);

#endif