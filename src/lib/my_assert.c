#include "lib/my_assert.h"
#include "lib/strings.h"

void 
assert_msg(const char *expr, const char *filename, int line, const char *function) {
    erroutf("%s:%d: %s: Assertion '(%s)' failed.\n", filename, line, function, expr);    
}