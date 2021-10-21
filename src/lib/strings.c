#include "lib/strings.h"

#include "lib/memory.h"
#include "lib/stream.h"

#include <stdlib.h>
#define STB_SPRINTF_IMPLEMENTATION
#include "thirdparty/stb_sprintf.h"

#include "thirdparty/utf8.h"

uptr vfmt(char *buf, uptr buf_size, const char *format, va_list args) {
    return stbsp_vsnprintf(buf, buf_size, format, args);
}

uptr fmt(char *buf, uptr buf_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    uptr result = vfmt(buf, buf_size, format, args);
    va_end(args);
    return result;
}

bool is_ascii(u32 symb) {
    return symb <= 0x7F;
}

bool is_space(u32 symb) {
    return symb == ' ' || symb == '\r' || symb == '\n';
}

bool is_nextline(u32 symb) {
    return (symb == '\n' || symb == '\r');  
}

bool is_digit(u32 symb) {
    return ('0' <= symb && symb <= '9');
}

bool is_alpha(u32 symb) {
    return ('a' <= symb && symb <= 'z') || ('A' <= symb && symb <= 'Z');
}

bool is_ident_start(u32 symb) {
    return is_alpha(symb) || symb == '_';
}

bool is_ident(u32 symb) {
    return is_alpha(symb) || is_digit(symb) || symb == '_';
}

bool is_printable(u32 symb) {
    return is_alpha(symb) || is_digit(symb) || is_punct(symb) || symb == ' ';    
}

bool is_punct(u32 symb) {
    return symb == '!' || symb == '\"' || symb == '#' || symb == '$' || symb == '%' || symb == '&' 
        || symb == '\'' || symb == '(' || symb == ')' || symb == '*' || symb == '+' || symb == ',' 
        || symb == '-' || symb == '.' || symb == '/' || symb == ':' || symb == ';' || symb == '<' 
        || symb == '=' || symb == '>' || symb == '?' || symb == '@' || symb == '[' || symb == '\\' 
        || symb == ']' || symb == '^' || symb == '_' || symb == '`' || symb == '{' || symb == '|' 
        || symb == '}' || symb == '~';
}

bool is_real(u32 symb) {
    return is_digit(symb) || symb == '-' || symb == '+' || symb == 'e' || symb == 'E' || symb == '.';
}

bool is_int(u32 symb) {
    return is_digit(symb) || symb == '-' || symb == '+';
}

f64 
z2f64(const char *str) {
    return atof(str);
}

i64
z2i64(const char *str) {
    return atoll(str);
}

uptr 
zcp(char *bf, uptr bf_sz, const char *str) {
    uptr bytes_to_copy = bf_sz;
    uptr len = zlen(str) + 1;
    if (len < bytes_to_copy) {
        bytes_to_copy = len;
    }
    mem_copy(bf, str, bytes_to_copy);
    return bytes_to_copy;
}

bool 
zeq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    bool result = *a == 0 && *b == 0;
    return result;
}

bool str_eqn(const char *a, const char *b, uptr n) {
    while (*a && *b && (*a == *b) && n--) {
        ++a;
        ++b;
    }
    bool result = n == 0;
    return result;
}

uptr zlen(const char *str) {
    uptr result = 0;
    while (*str++) {
        ++result;
    }
    return result;
}

u32 
utf8_encode(u32 utf32, u8 *dst) {
    u32 len = 0;
    if (utf32 <= 0x0000007F) {
        *dst = utf32;   
        len = 1;    
    } else if (utf32 <= 0x000007FF) {
        *dst++ = 0xC0 | (utf32 >> 6);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 2;
    } else if (utf32 <= 0x0000FFFF) {
        *dst++ = 0xE0 | (utf32 >> 12);
        *dst++ = 0x80 | ((utf32 >> 6) & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 3;
    } else if (utf32 <= 0x0010FFFF) {
        *dst++ = 0xF0 | (utf32 >> 18);
        *dst++ = 0x80 | ((utf32 >> 12) & 0x3F);
        *dst++ = 0x80 | ((utf32 >> 6) & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 4;
    } 
    return len;
}

const void *
utf8_decode(const void *buf, u32 *codepoint) {
#if 0
    u32 len = 0;
    u32 utf32 = 0;
    if ((src[0] & 0x80) == 0x00) {
        utf32 = src[0];
        len = 1;
    } else if ((src[0] & 0xE0) == 0xC0) {
        if ((src[1] & 0xC0) == 0x80) {
            utf32 = ((src[0] & 0x1F) << 6) | (src[1] & 0x3F);     
            len = 2; 
        }
    } else if ((src[0] & 0xF0) == 0xE0) {
        if ((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80) {
            utf32 = ((src[0] & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);     
            len = 3;
        }
    } else if ((src[0] & 0xF8) == 0xF0) {
        if ((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80) {
            utf32 = ((src[0] & 0x03) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) 
                | (src[3] & 0x3F);     
            len = 4;
        }
    }
    
    return utf32;
#else 
    // Credit - https://nullprogram.com/blog/2017/10/06/
    int errros;
    return __utf8_decode((void *)buf, codepoint, &errros);
#endif 
}

uptr outf(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    uptr result = outv(msg, args);
    va_end(args);
    return result;
}

uptr outv(const char *msg, va_list args) {
    Out_Stream *stream = get_stdout_stream();
    uptr result = out_streamv(stream, msg, args);
    out_stream_flush(stream);
    return result;
}

uptr erroutf(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    uptr result = erroutv(msg, args);
    va_end(args);
    return result;
}

uptr erroutv(const char *msg, va_list args) {
    Out_Stream *stream = get_stderr_stream();
    uptr result = out_streamv(stream, msg, args);
    out_stream_flush(stream);
    return result;
}

Str 
str(const char *data, u32 len) {
    Str text;
    text.data = data;
    text.len = len;
    return text;
}

bool 
str_eq(Str a, Str b) {
    bool result = (a.len == b.len) && mem_eq(a.data, b.data, a.len);
    return result;
}

bool 
str_startswith(Str a, Str b) {
    bool result = false;
    if (a.len >= b.len) {
        result = mem_eq(a.data, b.data, b.len);
    }
    return result;
}

bool 
str_endswith(Str a, Str b) {
    bool result = false;
    if (a.len >= b.len) {
        result = mem_eq(a.data + (a.len - b.len), b.data, b.len);
    }
    return result;
}

Str 
str_substr(Str a, u32 start, u32 end) {
    assert(end >= start);
    Str result = a;
    result.data += start;
    result.len = end - start;
    return result;
}

Str 
str_lstrip_(Str a, Str chars) {
    for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test = a.data[0];
        for (u32 i = 0; i < chars.len; ++i) {
            if (chars.data[i] == test) {
                is_found = true;
                break;
            }
        }
        
        if (is_found) {
            a = str_substr(a, 1, a.len);
        } else {
            break;
        }
    }
    return a;
}

Str 
str_rstrip_(Str a, Str chars) {
     for (;;) {
        if (!a.len) {
            break;
        }
        bool is_found = false;
        char test = a.data[a.len - 1];
        for (u32 i = 0; i < chars.len; ++i) {
            if (chars.data[i] == test) {
                is_found = true;
                break;
            }
        }
        
        if (is_found) {
            a = str_substr(a, 0, a.len - 1);
        } else {
            break;
        }
    }
    return a;
}

Str 
str_strip_(Str a, Str chars) {
    a = str_lstrip_(a, chars);
    a = str_rstrip_(a, chars);
    return a;
}

u32 
str_countc(Str str, char symb) {
    u32 result = 0;
    for (u32 i = 0; i < str.len; ++i) {
        if (str.data[i] == symb) {
            ++result;
        }
    }
    return 0;
}

u32 
str_count(Str str, Str substr) {
    NOT_IMPLEMENTED;
    return 0;
}

u32 
str_findc(Str a, char symb) {
    NOT_IMPLEMENTED;
    return 0;
}

u32 
str_find(Str a, Str substr) {
    u32 result = UINT32_MAX;
    if (a.len >= substr.len) {
        for (u32 i = 0; i < a.len - substr.len; ++i) {
            Str temp = str_substr(a, i, i + substr.len);
            if (str_eq(temp, substr)) {
                result = i;
                break;
            }   
        }
    }
    return result;
}

u32 
str_rfindc(Str a, char symb) {
    NOT_IMPLEMENTED;
    return 0;
}

u32 
str_rfind(Str a, Str substr) {
    u32 result = UINT32_MAX;
    if (a.len >= substr.len) {
        for (u32 i = 0; i < a.len - substr.len; ++i) {
            Str temp = str_substr(a, i, i + substr.len);
            if (str_eq(temp, substr)) {
                result = i;
            }   
        }
    }
    return result;
}

void 
str_lower(Str *strp) {
    NOT_IMPLEMENTED;
}

void 
str_upper(Str *strp) {
    NOT_IMPLEMENTED;
}

void 
str_capitalize(Str *strp) {
    NOT_IMPLEMENTED;
}
