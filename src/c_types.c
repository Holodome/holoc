#include "c_types.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

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

static void
convert_c_int(c_number_convert_result *result, char *p) {
    uint32_t base = 10;
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X') && isxdigit(p[2])) {
        p += 3;
        base = 16;
    } else if (*p == '0' && (p[1] == 'b' || p[1] == 'B') &&
               (p[2] == '0' || p[2] == '1')) {
        p += 3;
        base = 2;
    } else if (*p == '0') {
        ++p;
        base = 8;
    }

    uint64_t value     = strtoull(p, &p, base);

    enum { INT_SUF_L = 0x1, INT_SUF_LL = 0x2, INT_SUF_U = 0x4 };
    uint32_t suf = 0;
    if (strcmp(p, "LLU") == 0 || strcmp(p, "LLu") == 0 ||
        strcmp(p, "llU") == 0 || strcmp(p, "llu") == 0 ||
        strcmp(p, "ULL") == 0 || strcmp(p, "Ull") == 0 ||
        strcmp(p, "uLL") == 0 || strcmp(p, "ull") == 0) {
        p += 3;
        suf = INT_SUF_LL | INT_SUF_U;
    } else if (strcmp(p, "ll") == 0 || strcmp(p, "LL") == 0) {
        p += 2;
        suf = INT_SUF_LL;
    } else if (strcmp(p, "LU") == 0 || strcmp(p, "Lu") == 0 ||
               strcmp(p, "lU") == 0 || strcmp(p, "lu") == 0) {
        p += 2;
        suf = INT_SUF_L | INT_SUF_U;
    } else if (strcmp(p, "L") == 0 || strcmp(p, "l") == 0) {
        ++p;
        suf = INT_SUF_L;
    } else if (strcmp(p, "U") == 0 || strcmp(p, "u") == 0) {
        ++p;
        suf = INT_SUF_U;
    }

    if (!*p) {
        uint32_t type;
        if (base == 10) {
            if (suf == (INT_SUF_LL | INT_SUF_U)) {
                type = C_TYPE_ULLINT;
                ;
            } else if (suf == INT_SUF_LL) {
                type = C_TYPE_SLLINT;
            } else if (suf == (INT_SUF_L & INT_SUF_U)) {
                if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else if (suf == INT_SUF_L) {
                if (value <= LONG_MAX) {
                    type = C_TYPE_SLINT;
                } else {
                    type = C_TYPE_SLLINT;
                }
            } else if (suf == INT_SUF_U) {
                if (value <= UINT_MAX) {
                    type = C_TYPE_UINT;
                } else if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else {
                if (value <= INT_MAX) {
                    type = C_TYPE_SINT;
                } else if (value <= LONG_MAX) {
                    type = C_TYPE_SLINT;
                } else {
                    type = C_TYPE_SLLINT;
                }
            }
        } else {
            if (suf == (INT_SUF_LL | INT_SUF_U)) {
                type = C_TYPE_ULLINT;
                ;
            } else if (suf == INT_SUF_LL) {
                if (value <= LLONG_MAX) {
                    type = C_TYPE_SLLINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else if (suf == (INT_SUF_L & INT_SUF_U)) {
                if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else if (suf == INT_SUF_L) {
                if (value <= LONG_MAX) {
                    type = C_TYPE_SLINT;
                } else if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else if (value <= LLONG_MAX) {
                    type = C_TYPE_SLLINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else if (suf == INT_SUF_U) {
                if (value <= UINT_MAX) {
                    type = C_TYPE_UINT;
                } else if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else {
                if (value <= INT_MAX) {
                    type = C_TYPE_SINT;
                } else if (value <= UINT_MAX) {
                    type = C_TYPE_UINT;
                } else if (value <= LONG_MAX) {
                    type = C_TYPE_SLINT;
                } else if (value <= ULONG_MAX) {
                    type = C_TYPE_ULINT;
                } else if (value <= LLONG_MAX) {
                    type = C_TYPE_SLLINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            }
        }
        result->type_kind = type;
        result->uint_value = value;
        result->is_valid  = true;
    }
}

static void
convert_c_float(c_number_convert_result *result, char *p) {
    long double value = strtold(p, &p);
    uint32_t type = C_TYPE_DOUBLE;
    if (*p == 'f' || *p == 'F') {
        ++p;
        type = C_TYPE_FLOAT;
    } else if (*p == 'l' || *p == 'L') {
        ++p;
        type = C_TYPE_LDOUBLE;
    }

    if (!*p) {
        result->is_valid = true;
        result->float_value = value;
        result->type_kind = type;
    }
}

c_number_convert_result
convert_c_number(char *number) {
    c_number_convert_result result = {0};
    convert_c_int(&result, number);
    if (!result.is_valid) {
        convert_c_float(&result, number);
    }
    return result;
}
