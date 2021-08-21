#include "general.h"

#include "strings.h"
#include <stdio.h>

int main(int argc, char **argv) {
    u32 exp, mant;
    str_to_mantissa_exponent("12.375", 6, &exp, &mant);
    printf("Exited without errors\n");
    return 0;
}
