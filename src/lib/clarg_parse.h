// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/lib/clarg_parse.h
// Version: 0
// 
// Library for command-line arguments parsing
// @NOTE(hl): This API is not needed to be super fast, as it is executed only once at the start of the program
#pragma once
#include "general.h"
#include "platform/memory.h"

#define CLARG_NARG ((u32)-1)

enum {
    CLARG_TYPE_NONE = 0x0,
    CLARG_TYPE_STR,  // char *
    CLARG_TYPE_BOOL, // any size, assume little-endianess
    CLARG_TYPE_F64, 
    CLARG_TYPE_I64,  
};

typedef struct CLArgInfo {
    uptr output_offset; // offset in output structure
    const char *name;
    u32 narg; // if narg >= 1, data pointed at by output offset is treated as type *
    u32 type;
} CLArgInfo;

void clarg_parse(void *out_bf, CLArgInfo *infos, u32 ninfos, u32 argc, char ** const argv);