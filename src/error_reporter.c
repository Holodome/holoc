#include "error_reporter.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unicode.h>

#include "file_storage.h"
#include "str.h"

static error_reporter er_;
static error_reporter *er = &er_;

error_reporter *
get_error_reporter(void) {
    return er;
}

void
report_message_internalv(string file_contents, source_loc loc, string message_kind, char *msg,
                         va_list args) {
    char *file_eof        = STRING_END(file_contents);
    char *line_start      = file_contents.data;
    uint32_t line_counter = 1;
    while (line_counter < loc.line && line_start < file_eof) {
        if (*line_start == '\n') {
            ++line_counter;
        }
        ++line_start;
    }

    char *line_end = line_start + 1;
    while (*line_end != '\n' && line_end != file_eof) {
        ++line_end;
    }

    uint32_t utf8_col_counter = 0;
    char *utf8_col_cursor     = line_start;
    while (utf8_col_cursor < line_start + loc.col && utf8_col_cursor < line_end) {
        uint32_t _;
        utf8_col_cursor = utf8_decode(utf8_col_cursor, &_);
        ++utf8_col_counter;
    }

    fprintf(stderr, "\033[1m%.*s:%u:%u: %.*s: ", loc.filename.len, loc.filename.data,
            loc.line, utf8_col_counter, message_kind.len, message_kind.data);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\033[0m\n%.*s\n", (int)(line_end - line_start), line_start);
    if (utf8_col_counter != 1) {
        fprintf(stderr, "%*c", utf8_col_counter - 1, ' ');
    }
    fprintf(stderr, "\033[32;1m^\033[0m");
}

void
report_errorv(source_loc loc, char *fmt, va_list args) {
    file *f = fs_get_file(loc.filename, 0);
    report_message_internalv(f->contents, loc, WRAPZ("\033[31;1merror\033[1m"), fmt, args);
}

void
report_error(source_loc loc, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_errorv(loc, fmt, args);
}

void
report_warningv(source_loc loc, char *fmt, va_list args) {
    file *f = fs_get_file(loc.filename, 0);
    report_message_internalv(f->contents, loc, WRAPZ("\033[35;1mwarning\033[1m"), fmt, args);
}

void
report_warning(source_loc loc, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_warningv(loc, fmt, args);
}

void
report_notev(source_loc loc, char *fmt, va_list args) {
    file *f = fs_get_file(loc.filename, 0);
    report_message_internalv(f->contents, loc, WRAPZ("\033[90;1mnote\033[1m"), fmt, args);
}

void
report_note(source_loc loc, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_notev(loc, fmt, args);
}


