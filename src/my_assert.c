#include "my_assert.h"

#include <stdio.h>

void 
assert_msg(char *expr, char *filename, int line, char *function) {
    fprintf(stderr, "%s:%d: %s: Assertion '(%s)' failed.\n", filename, line, function, expr);    
}