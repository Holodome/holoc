#include "c_types.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "buffer_writer.h"

#define MAKE_TYPE(_kind, _size)      \
    &(c_type) {                      \
        .kind = _kind, .size = _size \
    }
static c_type *type_void = MAKE_TYPE(C_TYPE_VOID, 1);

static c_type *type_char   = MAKE_TYPE(C_TYPE_CHAR, 1);
static c_type *type_schar  = MAKE_TYPE(C_TYPE_SCHAR, 1);
static c_type *type_uchar  = MAKE_TYPE(C_TYPE_UCHAR, 1);
static c_type *type_wchar  = MAKE_TYPE(C_TYPE_WCHAR, 4);
static c_type *type_char16 = MAKE_TYPE(C_TYPE_CHAR16, 2);
static c_type *type_char32 = MAKE_TYPE(C_TYPE_CHAR32, 4);

static c_type *type_sint   = MAKE_TYPE(C_TYPE_SINT, 4);
static c_type *type_uint   = MAKE_TYPE(C_TYPE_UINT, 4);
static c_type *type_slint  = MAKE_TYPE(C_TYPE_SLINT, 8);
static c_type *type_ulint  = MAKE_TYPE(C_TYPE_ULINT, 8);
static c_type *type_sllint = MAKE_TYPE(C_TYPE_SLLINT, 8);
static c_type *type_ullint = MAKE_TYPE(C_TYPE_ULLINT, 8);
static c_type *type_ssint  = MAKE_TYPE(C_TYPE_SSINT, 2);
static c_type *type_usint  = MAKE_TYPE(C_TYPE_USINT, 2);

static c_type *type_float   = MAKE_TYPE(C_TYPE_FLOAT, 4);
static c_type *type_double  = MAKE_TYPE(C_TYPE_DOUBLE, 8);
static c_type *type_ldouble = MAKE_TYPE(C_TYPE_LDOUBLE, 8);

static c_type *type_bool = MAKE_TYPE(C_TYPE_BOOL, 1);

bool
c_type_is_int(c_type_kind kind) {
    return c_type_is_int_signed(kind) || c_type_is_int_unsigned(kind);
}

bool
c_type_is_float(c_type_kind kind) {
    return kind == C_TYPE_FLOAT || kind == C_TYPE_DOUBLE ||
           kind == C_TYPE_LDOUBLE;
}

bool
c_type_is_int_signed(c_type_kind kind) {
    return kind == C_TYPE_SCHAR || kind == C_TYPE_SINT ||
           kind == C_TYPE_SLINT || kind == C_TYPE_SLLINT ||
           kind == C_TYPE_SSINT;
}

bool
c_type_is_int_unsigned(c_type_kind kind) {
    return kind == C_TYPE_UCHAR || kind == C_TYPE_UINT ||
           kind == C_TYPE_ULINT || kind == C_TYPE_ULLINT ||
           kind == C_TYPE_USINT || kind == C_TYPE_CHAR ||
           kind == C_TYPE_CHAR16 || kind == C_TYPE_CHAR32 ||
           kind == C_TYPE_BOOL;
}

bool
c_types_are_compatible(c_type *a, c_type *b) {
    bool result = false;
    assert(false);
    // TODO:
    return result;
}

c_type *
get_standard_type(c_type_kind kind) {
    c_type *type = 0;
    switch (kind) {
    default:
        break;
    case C_TYPE_VOID:
        type = type_void;
        break;
    case C_TYPE_CHAR:
        type = type_char;
        break;
    case C_TYPE_SCHAR:
        type = type_schar;
        break;
    case C_TYPE_UCHAR:
        type = type_uchar;
        break;
    case C_TYPE_WCHAR:
        type = type_wchar;
        break;
    case C_TYPE_CHAR16:
        type = type_char16;
        break;
    case C_TYPE_CHAR32:
        type = type_char32;
        break;
    case C_TYPE_SINT:
        type = type_sint;
        break;
    case C_TYPE_UINT:
        type = type_uint;
        break;
    case C_TYPE_SLINT:
        type = type_slint;
        break;
    case C_TYPE_ULINT:
        type = type_ulint;
        break;
    case C_TYPE_SLLINT:
        type = type_sllint;
        break;
    case C_TYPE_ULLINT:
        type = type_ullint;
        break;
    case C_TYPE_SSINT:
        type = type_ssint;
        break;
    case C_TYPE_USINT:
        type = type_usint;
        break;
    case C_TYPE_FLOAT:
        type = type_float;
        break;
    case C_TYPE_DOUBLE:
        type = type_double;
        break;
    case C_TYPE_LDOUBLE:
        type = type_ldouble;
        break;
    case C_TYPE_BOOL:
        type = type_bool;
        break;
    }
    return type;
}

c_type *
make_ptr_type(c_type *base, struct allocator *a) {
    c_type *type = aalloc(a, sizeof(c_type));
    type->size   = sizeof(void *);
    type->ptr_to = base;
    type->kind   = C_TYPE_PTR;
    return type;
}

c_type *
make_array_type(c_type *base, uint32_t len, struct allocator *a) {
    c_type *type  = aalloc(a, sizeof(c_type));
    type->size    = len * base->size;
    type->ptr_to  = base;
    type->arr_len = len;
    type->kind    = C_TYPE_ARRAY;
    return type;
}

void
fmt_c_type_bw(c_type *type, buffer_writer *w) {
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
        fmt_c_type_bw(type->ptr_to, w);
        buf_write(w, "*");
    } break;
    case C_TYPE_FUNC: {
        fmt_c_type_bw(type->func_return, w);
        buf_write(w, "(");
        for (c_func_arg *arg = type->func_args; arg; arg = arg->next) {
            fmt_c_type_bw(arg->type, w);
            if (arg->next) {
                buf_write(w, ", ");
            }
        }
        buf_write(w, ")");
    } break;
    case C_TYPE_ARRAY: {
        fmt_c_type_bw(type->ptr_to, w);
        buf_write(w, "[%u]", type->arr_len);
    } break;
    case C_TYPE_UNION: {
        buf_write(w, "union");
        // Is this it?..
    } break;
    }
}

uint32_t
format_c_type(c_type *type, char *buf, uint32_t buf_size) {
    buffer_writer writer = {buf, buf + buf_size};
    fmt_c_type_bw(type, &writer);
    return writer.cursor - buf;
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

    uint64_t value = strtoull(p, &p, base);

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
            } else if (suf == INT_SUF_LL) {
                type = C_TYPE_SLLINT;
            } else if (suf == (INT_SUF_L | INT_SUF_U)) {
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
            } else if (suf == INT_SUF_LL) {
                if (value <= LLONG_MAX) {
                    type = C_TYPE_SLLINT;
                } else {
                    type = C_TYPE_ULLINT;
                }
            } else if (suf == (INT_SUF_L | INT_SUF_U)) {
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
        result->type_kind  = type;
        result->uint_value = value;
        result->is_valid   = true;
    }
}

static void
convert_c_float(c_number_convert_result *result, char *p) {
    long double value = strtold(p, &p);
    uint32_t type     = C_TYPE_DOUBLE;
    if (*p == 'f' || *p == 'F') {
        ++p;
        type = C_TYPE_FLOAT;
    } else if (*p == 'l' || *p == 'L') {
        ++p;
        type = C_TYPE_LDOUBLE;
    }

    if (!*p) {
        result->is_valid    = true;
        result->float_value = value;
        result->type_kind   = type;
    }
}

c_number_convert_result
convert_c_number(char *number) {
    c_number_convert_result result = {0};
    convert_c_int(&result, number);
    if (!result.is_valid) {
        convert_c_float(&result, number);
        result.is_float = result.is_valid;
    }
    return result;
}
