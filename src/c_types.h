#ifndef C_TYPES_H
#define C_TYPES_H

#include "general.h"

struct c_type;
struct buffer_writer;

typedef enum {
    C_TYPE_VOID = 0x0,  // void

    C_TYPE_CHAR   = 0x1,  // char
    C_TYPE_SCHAR  = 0x2,  // signed char
    C_TYPE_UCHAR  = 0x3,  // unsigned char
    C_TYPE_WCHAR  = 0x4,  // wchar_t
    C_TYPE_CHAR16 = 0x5,  // char16_t
    C_TYPE_CHAR32 = 0x6,  // char32_t

    C_TYPE_SINT   = 0x7,  // signed int
    C_TYPE_UINT   = 0x8,  // unsigned int
    C_TYPE_SLINT  = 0x9,  // signed long int
    C_TYPE_ULINT  = 0xA,  // unsigned long int
    C_TYPE_SLLINT = 0xB,  // signed long long int
    C_TYPE_ULLINT = 0xC,  // unsigned long long int
    C_TYPE_SSINT  = 0xD,  // signed short int
    C_TYPE_USINT  = 0xE,  // unsigned short int

    C_TYPE_FLOAT   = 0xF,   // float
    C_TYPE_DOUBLE  = 0x10,  // double
    C_TYPE_LDOUBLE = 0x11,  // long double

    C_TYPE_DECIMAL32  = 0x12,  // _Decimal32
    C_TYPE_DECIMAL64  = 0x13,  // _Decimal64
    C_TYPE_DECIMAL128 = 0x14,  // _Decimal128

    C_TYPE_BOOL = 0x15,  // _Bool

    C_TYPE_ENUM   = 0x16,  // enum
    C_TYPE_STRUCT = 0x17,  // struct
    C_TYPE_PTR    = 0x18,  // int*
    C_TYPE_FUNC   = 0x19,  // int(char, void**)
    C_TYPE_ARRAY  = 0x1A,  // int[10]
    C_TYPE_UNION  = 0x1B,  // union
} c_type_kind;

typedef struct c_struct_member {
    string name;
    struct c_type *type;
    uint32_t idx;
    uint64_t offset;

    struct c_struct_member *next;
} c_struct_member;

typedef struct c_func_arg {
    struct c_type *type;
    struct c_func_arg *next;
} c_func_arg;

typedef struct c_type {
    c_type_kind kind;
    uint64_t size;

    string name;

    c_struct_member *struct_members;
    struct c_type *ptr_to;
    uint32_t arr_len;
    struct c_type *func_return;
    c_func_arg *func_args;
} c_type;

bool c_type_kind_is_int(c_type_kind kind);
bool c_type_kind_is_float(c_type_kind kind);
bool c_type_kind_is_int_signed(c_type_kind kind);
bool c_type_kind_is_int_unsigned(c_type_kind kind);

bool c_type_are_compatible(c_type *a, c_type *b);
bool c_type_is_int(c_type *type);
c_type *get_standard_type(c_type_kind kind);
c_type *make_ptr_type(c_type *base);
c_type *make_array_type(c_type *base, uint32_t size);
c_type *make_c_type_struct(string name, c_struct_member *members);

void fmt_c_typew(c_type *type, struct buffer_writer *w);
uint32_t fmt_c_type(c_type *type, char *buf, uint32_t buf_size);

// Structure describing result of converting string to a number
typedef struct c_number_convert_result {
    // If given string has been identified as a number
    bool is_valid;
    // If set, result is written in float_value. Otherwise in uint_value
    bool is_float;
    // Parsed int value. Not unsigned here, as sign is not accounted here (sign
    // is another token in lexer)
    uint64_t uint_value;
    // Parsed float value
    long double float_value;
    // Resulting type
    c_type_kind type_kind;
} c_number_convert_result;

// NOTE: This is more of a function related to preprocessing (or lexing)
// but since it inderectly implies use of c_type_kind in result it has been
// decided to be put here
c_number_convert_result convert_c_number(char *number);

#endif
