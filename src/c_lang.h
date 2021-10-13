/*
Author: Holodome
Date: 12.10.2021
File: src/c_lang.h
Version: 0
*/
#ifndef C_LANG_H
#define C_LANG_H

#include "lib/general.h"

enum {
    C_TYPE_VOID   = 0x0, // void
    
    C_TYPE_CHAR   = 0x1, // char 
    C_TYPE_SCHAR  = 0x2, // signed char 
    C_TYPE_UCHAR  = 0x3, // unsigned char 
    C_TYPE_WCHAR  = 0x4, // wchar_t
    C_TYPE_CHAR16 = 0x5, // char16_t
    C_TYPE_CHAR32 = 0x6, // char32_t
    
    C_TYPE_SINT   = 0x7, // signed int
    C_TYPE_UINT   = 0x8, // unsigned int
    C_TYPE_SLINT  = 0x9, // signed long int
    C_TYPE_ULINT  = 0xA, // unsigned long int
    C_TYPE_SLLINT = 0xB, // signed long long int
    C_TYPE_ULLINT = 0xC, // unsigned long long int
    C_TYPE_SSINT  = 0xD, // signed short int
    C_TYPE_USINT  = 0xE, // unsigned short int
    
    C_TYPE_FLOAT   = 0xF,  // float
    C_TYPE_DOUBLE  = 0x10, // double
    C_TYPE_LDOUBLE = 0x11, // long double
    
    C_TYPE_DECIMAL32  = 0x12, // _Decimal32
    C_TYPE_DECIMAL64  = 0x13, // _Decimal64
    C_TYPE_DECIMAL128 = 0x14, // _Decimal128
  
    C_TYPE_BOOL = 0x15, // _Bool
    
    C_TYPE_ENUM   = 0x16, // enum
    C_TYPE_STRUCT = 0x17, // struct 
    C_TYPE_PTR    = 0x18, // *
    C_TYPE_FUNC   = 0x19, 
    C_TYPE_ARRAY  = 0x1A, 
    C_TYPE_UNION  = 0x1B, // union
};

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