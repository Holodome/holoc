/*
Author: Holodome
Date: 21.10.2021
File: src/type_table.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#define TYPE_TABLE_HASH_SIZE 1024

struct Src_Loc;
struct Compiler_Ctx;

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
    C_TYPE_ARRAY  = 0x1A, // int[10]
    C_TYPE_UNION  = 0x1B, // union
};

#define C_TYPE_IS_DECIMAL(_kind) \
(((_kind) == C_TYPE_DECIMAL32) || ((_kind) == C_TYPE_DECIMAL64) || ((_kind) == C_TYPE_DECIMAL128))
#define C_TYPE_IS_FLOAT(_kind) \
(((_kind) == C_TYPE_FLOAT) || ((_kind) == C_TYPE_DOUBLE) || ((_kind) == C_TYPE_LDOUBLE))
#define C_TYPE_IS_UNSIGNED_INT(_kind) \
(((_kind) == C_TYPE_UINT) || ((_kind) == C_TYPE_ULINT) || ((_kind) == C_TYPE_ULLINT) \
|| ((_kind) == C_TYPE_USINT) || ((_kind) == C_TYPE_UCHAR))
#define C_TYPE_IS_SIGNED_INT(_kind) \
(((_kind) == C_TYPE_SINT) || ((_kind) == C_TYPE_SLINT) || ((_kind) == C_TYPE_SLLINT) \
|| ((_kind) == C_TYPE_SSINT) || ((_kind) == C_TYPE_SCHAR))
#define C_TYPE_IS_INT(_kind) \
(C_TYPE_IS_SIGNED_INT(_kind) || C_TYPE_IS_UNSIGNED_INT(_kind) || ((_kind) == C_TYPE_BOOL) || ((_kind) == C_TYPE_CHAR))
#define C_TYPE_IS_NUMERIC(_kind) \
((u32)(_kind) <= C_TYPE_BOOL)
#define C_TYPE_IS_UNSIGNED(_kind) \
(C_TYPE_IS_UNSIGNED_INT(_kind) || ((_kind) == C_TYPE_PTR) || ((_kind) == C_TYPE_ARRAY))

typedef struct C_Struct_Member_Link {
    struct C_Struct_Member_Link *next;
    struct C_Struct_Member_Link *prev;
} C_Struct_Member_Link;

typedef struct C_Struct_Member {
    C_Struct_Member_Link link;
    const char *name;
    struct Src_Loc *decl_loc;
    
    struct C_Type *type;
    u32 index;  // index in structure
    u32 align;  // align 
    u32 offset; // byte offset
} C_Struct_Member;

typedef struct C_Type_Link {
    struct C_Type_Link *next;
    struct C_Type_Link *prev;
} C_Type_Link;

typedef struct C_Type {
    C_Type_Link link;
    u32 align;
    u32 size;  // value returned by sizeof operator
    
    u32 kind;
    struct Src_Loc *decl_loc;
    union {
        struct C_Type *ptr_to;
        struct {
            struct C_Type *array_base;
            u64 array_size;  
        };
        struct {
            bool is_variadic;
            struct C_Type *return_type;
            C_Type_Link param_sentinel;
            u32 param_count;
        };
        struct {
            bool is_packed;
            u32 member_count;   
            C_Struct_Member_Link member_sentinel;
        };
    };
} C_Type;

// Tags prefix types in C - for example, struct Foo and Foo are different types
enum {
    C_TYPE_TAG_NONE    = 0x0,
    C_TYPE_TAG_STRUCT  = 0x1,
    C_TYPE_TAG_ENUM    = 0x2,
    C_TYPE_TAG_UNION   = 0x3,
};

typedef struct Type_Table_Hash_Entry {
    struct Type_Table_Hash_Entry *next;
  
    u32         hash;
    u32         kind;
    const char *name;
    u32         scope;
    
    C_Type *type;
} Type_Table_Hash_Entry;

typedef struct Type_Table {
    struct Compiler_Ctx *ctx;
    
    C_Type *standard_types[0x16];
    Type_Table_Hash_Entry *type_hash[TYPE_TABLE_HASH_SIZE];
} Type_Table;

Type_Table *create_type_table(struct Compiler_Ctx *ctx);
// Returns C_Type corresponding to standard type
// (enum values 0x0 - 0x14)
C_Type *tt_get_standard_type(Type_Table *tt, u32 c_type);
void tt_get_type(Type_Table *tt, const char *name, u32 tag);
C_Type *tt_get_ptr(Type_Table *tt, C_Type *underlying);
void tt_remove_type(Type_Table *tt, const char *name);
C_Type *tt_make_typedef(Type_Table *tt, C_Type *type, u32 scope, const char *name);
C_Type *tt_get_new_type(Type_Table *tt, u32 tag, u32 scope, const char *name);
C_Type *tt_get_forward_decl(Type_Table *tt, u32 tag, const char *name);
// Does search for member respecting unnamed structs and unions
C_Struct_Member *get_struct_member(C_Type *struct_type, const char *name);
