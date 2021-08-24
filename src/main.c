#include "general.h"

#include <stdlib.h>
#include <stdio.h>
#include "tokenizer.h"
#include "parser.h"

int main(int argc, char **argv) {
    FILE *file = fopen("e:/new_dev/language/test.txt", "rb");
    fseek(file, 0, SEEK_END);
    uptr len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(len);
    fread(data, 1, len, file);
    
    Tokenizer tokenizer = create_tokenizer(data, len, "1");
    Parser parser = create_parser(&tokenizer);
    AST *ast = parse(&parser);
    (void)ast;
    
    printf("Exited without errors\n");
    return 0;
}
