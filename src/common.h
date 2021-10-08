/*
Author: Holodome
Date: 08.10.2021
File: src/common.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "lib/filesystem.h" // File_ID

typedef struct {
    u32 line;
    u32 symb;
    File_ID file;
} Src_Loc;

typedef struct {
    u64 value;
} String_ID;
