#include "interp.h"
#include "strings.h"
#include "parser.h"

void report_error(Interp *interp, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    erroutf("[ERROR] %s:", get_file_name(interp->file_id));
    verroutf(msg, args);
    erroutf("\n");    
}

void report_error_at_internal(Interp *interp, SourceLocation source_loc, const char *msg, va_list args) {
    const FileData *file_data = get_file_data(interp->file_id);
    
    u32 current_line_idx = 0;
    const char *line_start = file_data->str;
    while (current_line_idx != source_loc.line) {
        if (*line_start == '\n') {
            ++current_line_idx;
        }
        ++line_start;
    }
    const char *line_end = line_start;
    while (*line_end != '\n' || (line_end - file_data->str) < file_data->size) {
        ++line_end;
    }
    
    erroutf("[ERROR] %s:%u%u\n", get_file_name(interp->file_id), source_loc.line, source_loc.symb);
    erroutf("%.*s\n", line_end - line_start, line_start);
    erroutf("%*c^\n", source_loc.symb, ' ');
    erroutf("%*c", source_loc.symb, ' ');
    verroutf(msg, args);
    erroutf("\n");
}

void report_error_at(Interp *interp, SourceLocation source_loc, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, source_loc, msg, args);
}

void report_error_tok(Interp *interp, Token *token, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, token->source_loc, msg, args);
}

void report_error_ast(Interp *interp, AST *ast, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, ast->source_loc, msg, args);
        
}

Interp create_interp(const char *filename) {
    Interp interp = {0};
    interp.file_id = get_file_id_for_filename(filename);
    return interp;
}

void do_interp(Interp *interp) {
    Tokenizer tokenizer = create_tokenizer(interp->file_id);
    Parser parser = create_parser(&tokenizer, interp);
    AST *ast = parse(&parser);
    
}