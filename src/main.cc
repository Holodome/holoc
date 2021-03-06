#include "general.hh"
#include "mem.hh"
#include "str.hh"

#include "lexer.hh"
#include "ast.hh"
#include "interp.hh"

#include "lexer.cc"
#include "interp.cc"

struct File {
    Str name;
    void *data = 0;
    size_t data_size = 0;

    File(const Str &filename)
        : name(filename) {
        FILE *file = fopen(filename.data, "rb");
        assert(file);
        fseek(file, 0, SEEK_END);
        data_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        data = Mem::alloc(data_size + 1);
        fread(data, data_size, 1, file);
        ((char *)data)[data_size] = 0;
        
    }
    ~File() {
        Mem::free(data);
    }
};

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    Str filename = "test.txt";
    File file(filename);
    Lexer lexer;
    lexer.init(file.name, file.data, file.data_size);
    Parser parser(&lexer);
    Interp interp(parser.parse());

    printf("Exit\n");
    return 0;
}
