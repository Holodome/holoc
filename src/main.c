#include "general.h"

#include "strings.h"
#include <stdio.h>

#include "ieee754_convert.h"

int main(int argc, char **argv) {
    int a = ieee754_convert_check_correctness();
    assert(!a);
    printf("Exited without errors\n");
    return 0;
}
