#if !defined(UTF8_HH)

#include "general.hh"
#include "str.hh"

inline u32 utf8_encode(u32 utf32, u8 *dst) {
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
        *dst++ = 0x80 | (utf32 >> 6 & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 3;
    } else if (utf32 <= 0x0010FFFF) {
        *dst++ = 0xF0 | (utf32 >> 18);
        *dst++ = 0x80 | (utf32 >> 12 & 0x3F);
        *dst++ = 0x80 | (utf32 >> 6 & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 4;
    } else {
        assert(!"Invalid unicode value");
    }
    return len;
}

inline u32 utf8_decode(const u8 *src, const u8 **new_dst) {
    u32 len = 0;
    u32 utf32 = 0;
    if ((src[0] & 0x80) == 0x00) {
        utf32 = src[0];
        len = 1;
    } else if ((src[0] & 0xE0) == 0xC0) {
        assert((src[1] & 0xC0) == 0x80 && "Invalid UTF8 sequence");
        utf32 = (src[0] & 0x1F) << 6 | (src[1] & 0x3F);     
        len = 2; 
    } else if ((src[0] & 0xF0) == 0xE0) {
        assert((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && "Invalid UTF8 sequence");
        utf32 = (src[0] & 0x0F) << 12 | (src[1] & 0x3F) << 6 | (src[2] & 0x3F);     
        len = 3;
    } else if ((src[0] & 0xF8) == 0xF0) {
        assert((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80 && "Invalid UTF8 sequence");
        utf32 = (src[0] & 0x03) << 18 | (src[1] & 0x3F) << 12 | (src[2] & 0x3F) << 6 
            | (src[3] & 0x3F);     
        len = 4;
    }
    
    *new_dst = src + len;
    return utf32;
}

#define UTF8_HH 1
#endif 
