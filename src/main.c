#include "lib/general.h"

#include "lib/memory.h"
#include "lib/strings.h"

#include "lexer.h"

int 
main(int argc, char **argv) {
    Memory_Arena arena = {0};
    String_Storage *ss = create_string_storage(&arena);
    // Lexer *lexer = create_lexer(0, "examples/test.h");
    Lexer *lexer = create_lexer(0, "examples/tests/pp/include.h");
    for (;;) {
        Token *token = peek_tok(lexer);
        if (token->kind == TOKEN_EOS) {
            break;
        }
        char buf[4096];
        fmt_token(buf, sizeof(buf), ss, token);
        outf("%s\n", buf);
        eat_tok(lexer);
    }
    
    UNUSED(argc);
    UNUSED(argv);
    return 0;
}