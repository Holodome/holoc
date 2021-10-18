#include "error_reporter.h"

#include "lib/files.h"
#include "lib/stream.h"

#include "compiler_ctx.h"
#include "file_registry.h"
#include "lexer.h"
#include "ast.h"

void 
report_errorv(Error_Reporter *er, const Src_Loc *src_loc, const char *msg, va_list args) {
#if 0
    // const FileData *file_data = get_file_data(reporter->file_id);
    // @TODO capture source index and read only n first bytes instead of whole file
    File_ID id = src_loc.file_id;
    File_Data_Get_Result data_get = get_file_data(er->ctx->fr, id);
    assert(data_get.is_valid);
    File_Data data = data_get.data;
    // @TODO more robust algorithm
    const char *file_contents = (const char *)data.data;
    u64 file_size = data.size;
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
    const char *file_path = get_file_path(er->ctx->fr, id);
    Out_Stream *out = get_stderr_stream();
    out_streamf(out, "%s:%u:%u: \033[31merror\033[0m: ", file_path, src_loc.line, src_loc.symb);
    out_streamv(out, msg, args);
    // @TODO(hl): Investigate why printf "%*c" 0, ' ' gives one space still
    if (symb) {
        out_streamf(out, "\n%.*s\n%*c\033[33m^\033[0m\n", line_len, line_start,
            symb, ' ');
    } else {
        out_streamf(out, "\n%.*s\n\033[33m^\033[0m\n", line_len, line_start); 
    }
    out_stream_flush(out);
    ++er->error_count;
    DBG_BREAKPOINT;
#endif
}

void 
report_error(Error_Reporter *reporter, const Src_Loc *src_loc, const char *msg, ...) {
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
report_error_ast(Error_Reporter *reporter, Ast *ast, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_errorv(reporter, ast->src_loc, msg, args);
}
