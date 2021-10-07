#include "error_reporter.h"
#include "lexer.h"
#include "ast.h"

Error_Reporter *
create_error_reporter(Out_Stream *out, Out_Stream *errout, Memory_Arena *arena) {
    Error_Reporter *er = arena_alloc_struct(arena, Error_Reporter);
    er->arena = arena;
    er->out = out;
    er->errout = errout;
    return er;
}

bool 
is_error_reported(Error_Reporter *reporter) {
    return reporter->error_count != 0;
}

void 
report_errorv(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, va_list args) {
    // const FileData *file_data = get_file_data(reporter->file_id);
    // @TODO capture source index and read only n first bytes instead of whole file
    FileID id = src_loc.file;
    OS_File_Handle *handle = fs_get_handle(id);
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
    uptr line_end_offset = line_end - file_contents;
    while (*line_end != '\n' && line_end_offset < file_size) {
        ++line_end;
    }
    int line_len = line_end - line_start;
    assert(src_loc.symb);
    u32 symb = src_loc.symb - 1;
    char filename[4096];
    fs_fmt_filename(filename, sizeof(filename), id);
    Out_Stream *out = get_stderr_stream();
    out_streamf(out, "%s:%u:%u: \033[31merror\033[0m: ", filename, src_loc.line, src_loc.symb);
    out_streamv(out, msg, args);
    // @TODO(hl): Investigate why printf "%*c" 0, ' ' gives one space still
    if (symb) {
        out_streamf(out, "\n%.*s\n%*c\033[33m^\033[0m\n", line_len, line_start,
            symb, ' ');
    } else {
        out_streamf(out, "\n%.*s\n\033[33m^\033[0m\n", line_len, line_start); 
    }
    out_stream_flush(out);
    ++reporter->error_count;
    DBG_BREAKPOINT;
}

void 
report_error(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, src_loc, msg, args);
}

void 
report_error_tok(Error_Reporter *reporter, Token *tok, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, tok->src_loc, msg, args);
}

void 
report_error_ast(Error_Reporter *reporter, AST *ast, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, ast->src_loc, msg, args);
}

void 
report_error_generalv(const char *msg, va_list args) {
    Out_Stream *stream = get_stderr_stream();
    out_streamf(stream, "\033[31merror\033[0m: ");
    out_streamv(stream, msg, args);
    out_streamf(stream, "\n");
    out_stream_flush(stream);
}

void 
report_error_general(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_generalv(msg, args);
}

void 
print_reporter_summary(Error_Reporter *reporter) {
    if (is_error_reported(reporter)) {
        erroutf("Compilation failed. %u errors encountered\n", reporter->error_count);
    } else {
        outf("Compilation successfull\n");
    }
}