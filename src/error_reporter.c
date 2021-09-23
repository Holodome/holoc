#include "error_reporter.h"

ErrorReporter *create_error_reporter(const char *filename) {
    ErrorReporter *reporter = arena_bootstrap(ErrorReporter, arena);
    reporter->filename = arena_alloc_str(&reporter->arena, filename);
    return reporter;
}

void destroy_error_reporter(ErrorReporter *reporter) {
    arena_clear(&reporter->arena);
}

b32 is_error_reported(ErrorReporter *reporter) {
    return reporter->error_count != 0;
}

void report_errorv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args) {
    // const FileData *file_data = get_file_data(reporter->file_id);
    // @TODO capture source index and read only n first bytes instead of whole file
    FileHandle file_handle = {0};
    open_file(&file_handle, reporter->filename, FILE_MODE_READ);
    uptr file_size = get_file_size(&file_handle);
    char *file_contents = arena_alloc(&reporter->arena, file_size);
    read_file(&file_handle, 0, file_contents, file_size);
    // @TODO more robust algorithm
    u32 current_line_idx = 1;
    const char *line_start = file_contents;
    while (current_line_idx != src_loc.line) {
        if (*line_start == '\n') {
            ++current_line_idx;
        }
        ++line_start;
    }
    const char *line_end = line_start;
    while (*line_end != '\n' && (line_end - file_contents) < file_size) {
        ++line_end;
    }
    
    // @TODO
    // erroutf("%s:%u:%u: \033[31merror\033[0m: ", get_file_name(reporter->file_id), src_loc.line, src_loc.symb);
    erroutv(msg, args);
    erroutf("\n%.*s\n%*c\033[33m^\033[0m\n", line_end - line_start, line_start,
        src_loc.symb, ' ');
    
    ++reporter->error_count;
}

void report_error(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, src_loc, msg, args);
}

void report_error_tok(ErrorReporter *reporter, Token *tok, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, tok->src_loc, msg, args);
}

void report_error_ast(ErrorReporter *reporter, AST *ast, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, ast->src_loc, msg, args);
        
}

void report_error_generalv(const char *msg, va_list args) {
    OutStream *st = get_stderr_stream();
    out_streamf(st, "\033[31merror\033[0m: ");
    out_streamv(st, msg, args);
    out_streamf(st, "\n");
}

void report_error_general(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_generalv(msg, args);
}