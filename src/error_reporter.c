#include "error_reporter.h"
#include "tokenizer.h"
#include "ast.h"

b32 is_error_reported(ErrorReporter *reporter) {
    return reporter->error_count != 0;
}

void report_errorv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args) {
    // const FileData *file_data = get_file_data(reporter->file_id);
    // @TODO capture source index and read only n first bytes instead of whole file
    FileID id = src_loc.file;
    OSFileHandle *handle = fs_get_handle(id);
    assert(handle);
    uptr file_size = fs_get_file_size(id);
    char *file_contents = mem_alloc(file_size);
    os_read_file(handle, 0, file_contents, file_size);
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
    
    assert(src_loc.symb);
    u32 symb = src_loc.symb - 1;
    char filename[4096];
    fs_fmt_filename(filename, sizeof(filename), id);
    erroutf("%s:%u:%u: \033[31merror\033[0m: ", filename, src_loc.line, src_loc.symb);
    erroutv(msg, args);
    // @TODO(hl): Investigate why printf %*c 0, ' ' gives one space still
    if (symb) {
        erroutf("\n%.*s\n%*c\033[33m^\033[0m\n", line_end - line_start, line_start,
            symb, ' ');
    } else {
        erroutf("\n%.*s\n\033[33m^\033[0m\n", line_end - line_start, line_start); 
    }
    ++reporter->error_count;
    DBG_BREAKPOINT;
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
    out_stream_flush(st);
}

void report_error_general(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_generalv(msg, args);
}