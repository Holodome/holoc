#include "unicode.h"

/* Decode the next character, C, from BUF, reporting errors in E.
 *
 * Since this is a branchless decoder, four bytes will be read from the
 * buffer regardless of the actual length of the next character. This
 * means the buffer _must_ have at least three bytes of zero padding
 * following the end of the data stream.
 *
 * Errors are reported in E, which will be non-zero if the parsed
 * character was somehow invalid: invalid byte sequence, non-canonical
 * encoding, or a surrogate half.
 *
 * The function returns a pointer to the next character. When an error
 * occurs, this pointer will be a guess that depends on the particular
 * error, but it will always advance at least one byte.
 */
static void *
__utf8_decode(void *buf, uint32_t *c, int *e)
{
    static const char lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };
    static const int masks[]  = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
    static const uint32_t mins[] = {4194304, 0, 128, 2048, 65536};
    static const int shiftc[] = {0, 18, 12, 6, 0};
    static const int shifte[] = {0, 6, 4, 2, 0};

    unsigned char *s = buf;
    int len = lengths[s[0] >> 3];

    /* Compute the pointer to the next character early so that the next
     * iteration can start working on the next character. Neither Clang
     * nor GCC figure out this reordering on their own.
     */
    unsigned char *next = s + len + !len;

    /* Assume a four-byte character and load four bytes. Unused bits are
     * shifted out.
     */
    *c  = (uint32_t)(s[0] & masks[len]) << 18;
    *c |= (uint32_t)(s[1] & 0x3f) << 12;
    *c |= (uint32_t)(s[2] & 0x3f) <<  6;
    *c |= (uint32_t)(s[3] & 0x3f) <<  0;
    *c >>= shiftc[len];

    /* Accumulate the various error conditions. */
    *e  = (*c < mins[len]) << 6; // non-canonical encoding
    *e |= ((*c >> 11) == 0x1b) << 7;  // surrogate half?
    *e |= (*c > 0x10FFFF) << 8;  // out of range?
    *e |= (s[1] & 0xc0) >> 2;
    *e |= (s[2] & 0xc0) >> 4;
    *e |= (s[3]       ) >> 6;
    *e ^= 0x2a; // top two bits of each tail byte correct?
    *e >>= shifte[len];

    return next;
}

void *
utf8_decode_verbose(void *src, uint32_t *codepoint_p, uint32_t *errors) {
    return __utf8_decode(src, codepoint_p, (int *)errors);   
}

void *
utf8_decode(void *src, uint32_t *codepoint_p) {
    uint32_t error;
    return utf8_decode_verbose(src, codepoint_p, &error);    
}

void *
utf8_encode(void *dst, uint32_t codepoint) {
    uint8_t *d = (uint8_t *)dst; 
    if (codepoint <= 0x0000007F) {
        *d++ = codepoint;   
    } else if (codepoint <= 0x000007FF) {
        *d++ = 0xC0 | (codepoint >> 6);
        *d++ = 0x80 | (codepoint & 0x3F);
    } else if (codepoint <= 0x0000FFFF) {
        *d++ = 0xE0 | (codepoint >> 12);
        *d++ = 0x80 | ((codepoint >> 6) & 0x3F);
        *d++ = 0x80 | (codepoint & 0x3F);
    } else if (codepoint <= 0x0010FFFF) {
        *d++ = 0xF0 | (codepoint >> 18);
        *d++ = 0x80 | ((codepoint >> 12) & 0x3F);
        *d++ = 0x80 | ((codepoint >> 6) & 0x3F);
        *d++ = 0x80 | (codepoint & 0x3F);
    } 
    return (void *)d;
}

void *
utf16_encode(void *dst, uint32_t codepoint) {
    uint16_t *d = (uint16_t *)dst;
    if (codepoint < 0x10000) {
        *d++ = (uint16_t)codepoint;
    } else {
        codepoint -= 0x10000;
        *d++ = 0xD800 + ((codepoint >> 10) & 0x3FF);
        *d++ = 0xDC00 + (codepoint & 0x3FF);
    }
    return d;
}
