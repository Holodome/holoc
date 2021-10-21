/*
Author: Holodome
Date: 12.10.2021
File: src/c_lang.h
Version: 0
*/
#ifndef C_LANG_H
#define C_LANG_H

#include "lib/general.h"

struct C_Type;

typedef struct C_Struct_Member {
    struct C_Type *type;
    u32 index;
    u32 offset;
    
    struct C_Struct_Member *next;
} C_Struct_Member;

typedef struct C_Type {
    u32 kind;
    u64 size; // sizeof
    
    union {
        C_Struct_Member *members;
    }; 
} C_Type;

#endif