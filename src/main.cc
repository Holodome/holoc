#include "general.hh"
#include "mem.hh"
#include "str.hh"

#include "lexer.hh"
#include "ast.hh"
#include "interp.hh"

#include "lexer.cc"
#include "parser.cc"
#include "interp.cc"

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    Str filename = "test.txt";

    Interp interp(filename);
    interp.interp();

    printf("Exit\n");
    return 0;
}
