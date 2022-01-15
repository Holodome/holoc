#include "c_types.h"

#include "buffer_writer.h"

void
fmt_c_type(c_type *type, buffer_writer *w) {
    switch (type->kind) {
    case C_TYPE_VOID: {
        buf_write(w, "void");
    } break;
    case C_TYPE_CHAR: {
        buf_write(w, "char");
    } break;
    case C_TYPE_SCHAR: {
        buf_write(w, "signed char");
    } break;
    case C_TYPE_UCHAR: {
        buf_write(w, "unsigned char");
    } break;
    case C_TYPE_WCHAR: {
        buf_write(w, "wchar_t");
    } break;
    case C_TYPE_CHAR16: {
        buf_write(w, "char16_t");
    } break;
    case C_TYPE_CHAR32: {
        buf_write(w, "char32_t");
    } break;
    case C_TYPE_SINT: {
        buf_write(w, "signed int");
    } break;
    case C_TYPE_UINT: {
        buf_write(w, "unsigned int");
    } break;
    case C_TYPE_SLINT: {
        buf_write(w, "signed long int");
    } break;
    case C_TYPE_ULINT: {
        buf_write(w, "unsigned long int");
    } break;
    case C_TYPE_SLLINT: {
        buf_write(w, "signed long long int");
    } break;
    case C_TYPE_ULLINT: {
        buf_write(w, "unsigned long long int");
    } break;
    case C_TYPE_SSINT: {
        buf_write(w, "signed short int");
    } break;
    case C_TYPE_USINT: {
        buf_write(w, "unsigned short int");
    } break;
    case C_TYPE_FLOAT: {
        buf_write(w, "float");
    } break;
    case C_TYPE_DOUBLE: {
        buf_write(w, "double");
    } break;
    case C_TYPE_LDOUBLE: {
        buf_write(w, "long double");
    } break;
    case C_TYPE_DECIMAL32: {
        buf_write(w, "_Decimal32");
    } break;
    case C_TYPE_DECIMAL64: {
        buf_write(w, "_Decimal64");
    } break;
    case C_TYPE_DECIMAL128: {
        buf_write(w, "_Decimal128");
    } break;
    case C_TYPE_BOOL: {
        buf_write(w, "_Bool");
    } break;
    case C_TYPE_ENUM: {
        buf_write(w, "enum");
        // Is this it?..
    } break;
    case C_TYPE_STRUCT: {
        buf_write(w, "struct");
        // Is this it?..
    } break;
    case C_TYPE_PTR: {
        fmt_c_type(type->ptr_to, w);
        buf_write(w, "*");
    } break;
    case C_TYPE_FUNC: {
        fmt_c_type(type->func_return, w);
        buf_write(w, "(");
        for (c_func_arg *arg = type->func_args; arg; arg = arg->next) {
            fmt_c_type(arg->type, w);
            if (arg->next) {
                buf_write(w, ", ");
            }
        }
        buf_write(w, ")");
    } break;
    case C_TYPE_ARRAY: {
        fmt_c_type(type->ptr_to, w);
        buf_write(w, "[%u]", type->array_len);
    } break;
    case C_TYPE_UNION: {
        buf_write(w, "union");
        // Is this it?..
    } break;
    }
}
