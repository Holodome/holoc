#include "type_table.h"

#include "lib/hashing.h"
#include "lib/strings.h"
#include "lib/memory.h"
#include "lib/lists.h"

#include "compiler_ctx.h"

#define TT_STR_HASH_FUNC fnv64

static Type_Table_Hash_Entry **
tt_get_hash_entry(Type_Table *table, u32 hash) {
    CT_ASSERT(IS_POW2(TYPE_TABLE_HASH_SIZE));
    u32 hash_value = hash & TYPE_TABLE_HASH_SIZE;
    Type_Table_Hash_Entry **hash_entry = table->type_hash + hash_value;
    while (*hash_entry && ((*hash_entry)->hash != hash)) {
        hash_entry = &(*hash_entry)->next;
    }   
    return hash_entry;
}

static u32 
hash_type(const char *name, u32 kind) {
    // @TODO(hl): Better hash func
    const static u32 kind_masks[] = {
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
        0xFFFF0000,
    };
    u32 name_hash = fnv64(name, zlen(name));
    u32 result_hash = name_hash ^ kind_masks[kind];
    return result_hash;
}


#define get_new_type(_table, _name, ...) get_new_type_(_table, _name, (0, ##__VA_ARGS__))
static C_Type *
get_new_type_(Type_Table *tt, const char *name, u32 kind) {
    // u32 type_hash = hash_type(name, kind);
    // Type_Table_Hash_Entry **table_entry = tt_get_hash_entry(tt, type_hash);
    // if (*table_entry) {
        
    // } else {
        
    // }
    // C_Type *new_type = arena_alloc_struct(tt->ctx->arena, C_Type);
    // assert(*typep);
    // *typep = new_type;
    // return new_type;
    return 0;
}

static C_Type *
init_type(C_Type *type, u32 kind, u32 align, u32 size) {
    type->kind = kind;
    type->align = align;
    type->size = size;
    return type;
}

static void 
initialize_standard_types(Type_Table *tt) {
    // @NOTE(hl): Although we add strings here, they should be never
    // accessed because of the nature of lexer (all standard types)
    // are keywords so require special handling
    tt->standard_types[C_TYPE_VOID] = init_type(
        get_new_type(tt, "void"), C_TYPE_VOID, 1, 1
    );
    tt->standard_types[C_TYPE_CHAR] = init_type(
        get_new_type(tt, "char"), C_TYPE_CHAR, 1, 1
    );
    tt->standard_types[C_TYPE_SCHAR] = init_type(
        get_new_type(tt, "signed char"), C_TYPE_SCHAR, 1, 1
    );
    tt->standard_types[C_TYPE_UCHAR] = init_type(
        get_new_type(tt, "unsigned char"), C_TYPE_UCHAR, 1, 1
    );
    tt->standard_types[C_TYPE_WCHAR] = init_type(
        get_new_type(tt, "wchar_t"), C_TYPE_WCHAR, 4, 4
    );
    tt->standard_types[C_TYPE_CHAR16] = init_type(
        get_new_type(tt, "char16_t"), C_TYPE_CHAR16, 2, 2
    );
    tt->standard_types[C_TYPE_CHAR32] = init_type(
        get_new_type(tt, "char32_t"), C_TYPE_CHAR32, 4, 4
    );
    
    tt->standard_types[C_TYPE_SINT] = init_type(
        get_new_type(tt, "signed int"), C_TYPE_SINT, 4, 4
    );
    tt->standard_types[C_TYPE_UINT] = init_type(
        get_new_type(tt, "unsigned int"), C_TYPE_UINT, 4, 4
    );
    tt->standard_types[C_TYPE_SLINT] = init_type(
        get_new_type(tt, "signed long int"), C_TYPE_SLINT, 8, 8
    );
    tt->standard_types[C_TYPE_ULINT] = init_type(
        get_new_type(tt, "unsigned long int"), C_TYPE_ULINT, 8, 8
    );
    tt->standard_types[C_TYPE_SLLINT] = init_type(
        get_new_type(tt, "signed long long int"), C_TYPE_SLINT, 8, 8
    );
    tt->standard_types[C_TYPE_ULLINT] = init_type(
        get_new_type(tt, "unsigned long long int"), C_TYPE_ULLINT, 8, 8
    );
    tt->standard_types[C_TYPE_SSINT] = init_type(
        get_new_type(tt, "signed short int"), C_TYPE_SSINT, 2, 2
    );
    tt->standard_types[C_TYPE_USINT] = init_type(
        get_new_type(tt, "unsigned short int"), C_TYPE_USINT, 2, 2
    );
    tt->standard_types[C_TYPE_FLOAT] = init_type(
        get_new_type(tt, "float"), C_TYPE_FLOAT, 4, 4
    );
    tt->standard_types[C_TYPE_DOUBLE] = init_type(
        get_new_type(tt, "double"), C_TYPE_DOUBLE, 8, 8
    );
    tt->standard_types[C_TYPE_LDOUBLE] = init_type(
        get_new_type(tt, "long double"), C_TYPE_CHAR, 16, 16
    );
    
    tt->standard_types[C_TYPE_DECIMAL32] = init_type(
        get_new_type(tt, "_Decimal32"), C_TYPE_DECIMAL32, 4, 4
    );
    tt->standard_types[C_TYPE_DECIMAL64] = init_type(
        get_new_type(tt, "_Decimal64"), C_TYPE_DECIMAL64, 8, 8
    );
    tt->standard_types[C_TYPE_DECIMAL128] = init_type(
        get_new_type(tt, "_Decimal128"), C_TYPE_DECIMAL128, 16, 16
    );
    
    tt->standard_types[C_TYPE_BOOL] = init_type(
        get_new_type(tt, "_Bool"), C_TYPE_BOOL, 1, 1
    );
}

Type_Table *
create_type_table(struct Compiler_Ctx *ctx) {
    Type_Table *tt = arena_alloc_struct(ctx->arena, Type_Table);
    tt->ctx = ctx;  
    initialize_standard_types(tt);
    return tt;
}

C_Type *
tt_get_standard_type(Type_Table *tt, u32 c_type) {
    C_Type *type = 0;
    
    return type;    
}

C_Type *
tt_get_untagged(Type_Table *tt, const char *name) {
    C_Type *type = 0;
    
    return type;
}

C_Type *
tt_get_struct(Type_Table *tt, const char *name) {
    C_Type *type = 0;
    
    return type;
}

C_Type *
tt_get_union(Type_Table *tt, const char *name) {
    C_Type *type = 0;
    
    return type;
}

C_Type *
tt_get_enum(Type_Table *tt, const char *name) {
    C_Type *type = 0;
    
    return type;
}

C_Type *
tt_get_ptr(Type_Table *tt, C_Type *underlying) {
    C_Type *type = 0;
    
    return type;
}

C_Type *
tt_make_typedef(Type_Table *tt, C_Type *underlying, const char *name) {
    C_Type *type = 0;
    
    return type;
}
// Does search for member respecting unnamed structs and unions
C_Struct_Member *
get_struct_member(C_Type *struct_type, const char *name) {
    C_Struct_Member *type = 0;
    
    return type;
}
