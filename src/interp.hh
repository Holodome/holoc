#if !defined(INTERP_HH)

#include "lexer.hh"
#include "ast.hh"
#include "parser.hh"

// Struct for interpreting single program instance
struct Interp {
    // Current file name
    Str filename;
    // Current file data
    u8 *file_data;
    size_t file_data_size;
    // Lexer initialized for parsing current file.
    // Owns cursor and is used to get current location in source
    Lexer lexer;
    // 
    Parser parser;
    // If error was reported, we should abort program safely
    bool reported_error = false;
    
    Interp(const Str &filename);
    ~Interp();

    // Interp current file
    void interp();
    
    void report_error(Token *token, const char *format, ...);
    void report_error(Ast *ast, const char *format, ...);
    void report_error(const char *format, ...);
};

#define INTERP_HH 1
#endif
