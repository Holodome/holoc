// ieee754_convert.h - convert from strings to iee754 defined floating-point formats
// 
// to build this, in one source file that includes this file do 
//    #define IEEE754_CONVERT_IMPLEMENTATION
// this program parses a string with real value and returns its ieee754 representation in one of the described formats
// Behaviour is similar to atof function, but with support for different formats and (possibly)
// greater perfomance, as well as support for not zero-terminated strings.
//
// No libraries used.
// 
// Notes: to expand this library to support formats of larger sizes, support of larger integers
// should be added. It is not clear how conversion could be abstracted then without perfomance loss,
// because all data types should be different per format.
// Also it does not handle any special cases, like infinities and too large or small values, and does 
// zero error handling

#ifndef IEE754_CONVERT_H
#define IEE754_CONVERT_H 1

// Floating-point formats of different widths
enum ieee754_convert_format {
    IEEE754_CONVERT_32BIT,
    IEEE754_CONVERT_64BIT,
};

float ieee754_convert_f32(const char *str, int unsigned length);
double ieee754_convert_f64(const char *str, int unsigned length);

// Returns 0 if all tests passed
int ieee754_convert_check_correctness();

#endif 

#ifdef IEEE754_CONVERT_IMPLEMENTATION

typedef unsigned long ieee754_convert_u32;
typedef unsigned long long ieee754_convert_u64;
typedef long long ieee754_convert_i64;

typedef struct {
    int sign_bit;
    int exponent_bits;
    int mantissa_bits;
} ieee754_convert_format_info;

static ieee754_convert_format_info ieee754_convert_format_infos[] = {
    {31, 8, 23},
    {63, 11, 52}  
};

static void ieee754_convert_abstracted(const char *str, int unsigned length, 
    int format, void *out) {
    ieee754_convert_format_info info = ieee754_convert_format_infos[format];
    
    ieee754_convert_u64 integer_part = 0;
    ieee754_convert_u64 fraction_part_dec = 0;
    ieee754_convert_u64 fraction_part_dec_delimeter = 1;

    int dot_is_passed = 0;
    // Note that although signs are handled here, usage code in tokenizer does not provide this function numbers with sign
    int is_negative = 0;
    while (length--) {
        char symb = *str++;
        if (symb == '-') {
            is_negative = 1;
        } else if (symb == '+') {
        } else if ('0' <= symb && symb <= '9') {
            if (dot_is_passed) {
                fraction_part_dec = fraction_part_dec * 10 + (symb - '0');
                fraction_part_dec_delimeter *= 10;
            } else {
                integer_part = integer_part * 10 + (symb - '0');
            }
        } else if (symb == '.') {
            dot_is_passed = 1;
        } else {
            break;
        }
    }
    
    // construct binary representation of fraction
    ieee754_convert_u64 fraction_part = 0;
    ieee754_convert_u64 fraction_part_bitlength = 0;
    while (fraction_part_bitlength < info.mantissa_bits) {
        ++fraction_part_bitlength;
        fraction_part_dec *= 2;
        fraction_part = (fraction_part << 1) | (fraction_part_dec / fraction_part_dec_delimeter);
        fraction_part_dec %= fraction_part_dec_delimeter;
        if (fraction_part_dec == 0) {
            break;
        }
    }
    // construct combined binary representation
    ieee754_convert_u64 combined = (integer_part << fraction_part_bitlength) | fraction_part;
    ieee754_convert_u64 combined_point_index = fraction_part_bitlength; // counting from right
    // normalize binary representation
    ieee754_convert_i64 exponent = 0;
    while ((combined >> combined_point_index) > 1) {
        ++combined_point_index;
        ++exponent;
    } 
    while ((combined >> combined_point_index) == 0) {
        --exponent;
        --combined_point_index;
    }
    // now: data for IEEE 754 representation:
    // sign = SIGN_BIT * is_negative
    // exponent (unajusted) = exponent
    // mantissa (not normalized) = combined without leading 1 (only part after point)
    //
    // adjust the exponent
    ieee754_convert_u64 exponent_bias = (1 << (info.exponent_bits - 1)) - 1;
    ieee754_convert_u64 exponent_biased = exponent + exponent_bias;
    // normalize the mantissa
    // remove leading 1
    ieee754_convert_u64 mantissa = combined & ((1llu << combined_point_index) - 1);
    // adjust length of mantissa
    while (combined_point_index < info.mantissa_bits) {
        combined_point_index++;
        mantissa <<= 1;
    }
    // construct the IEEE 754 representation using sign, exponent and mantissa
    ieee754_convert_u64 ieee = ((1 << info.sign_bit) * is_negative) | (exponent_biased << info.mantissa_bits) | (mantissa);
    if (format == IEEE754_CONVERT_32BIT) {
        ieee754_convert_u32 truncated = (ieee754_convert_u32)ieee;
        *(ieee754_convert_u32 *)out = truncated;
    } else if (format == IEEE754_CONVERT_64BIT) {
        *(ieee754_convert_u64 *)out = ieee; 
    }
}
    
float ieee754_convert_f32(const char *str, int unsigned length) {
    float result = 0;
    ieee754_convert_abstracted(str, length, IEEE754_CONVERT_32BIT, &result);
    return result;
}

double ieee754_convert_f64(const char *str, int unsigned length) {
    double result = 0;
    ieee754_convert_abstracted(str, length, IEEE754_CONVERT_64BIT, &result);
    return result;    
}

static double ieee754_convert_abs_f64(double a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

#define STR_WITH_LEN(_str) #_str, sizeof(#_str)
#define TEST(_n) (ieee754_convert_abs_f64(ieee754_convert_f64(STR_WITH_LEN(_n)) - _n) > 0.00001)
#define TEST1(_n) (ieee754_convert_abs_f64(ieee754_convert_f32(STR_WITH_LEN(_n)) - _n) > 0.001)
int ieee754_convert_check_correctness() {
    int result = 0;
    result += TEST(1);
    result += TEST(2);
    result += TEST(3);
    result += TEST(4);
    result += TEST(5);
    result += TEST(6);
    result += TEST(12.375);
    result += TEST(0.375);
    result += TEST(0.25);
    result += TEST(0.333333);
    result += TEST1(1);
    result += TEST1(2);
    result += TEST1(3);
    result += TEST1(4);
    result += TEST1(5);
    result += TEST1(6);
    result += TEST1(12.375);
    result += TEST1(0.375);
    result += TEST1(0.25);
    result += TEST1(0.333);
    return result;
}


#endif 