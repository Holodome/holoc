#include "file_storage.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "str.h"

char *
canonicalize_newline(char *p) {
    char *read  = p;
    char *write = p;
    while (*read) {
        if (read[0] == '\r' && read[1] == '\n') {
            read += 2;
            *write++ = '\n';
        } else if (*read == '\r') {
            ++read;
            *write++ = '\n';
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
    return write;
}

char *
replace_trigraphs(char *p) {
    char *read  = p;
    char *write = p;
    while (*read) {
        if (read[0] == '?' && read[1] == '?') {
            switch (read[2]) {
            default: {
                *write++ = *read++;
                *write++ = *read++;
            } break;
            case '<': {
                read += 3;
                *write++ = '{';
            } break;
            case '>': {
                read += 3;
                *write++ = '}';
            } break;
            case '(': {
                read += 3;
                *write++ = '[';
            } break;
            case ')': {
                read += 3;
                *write++ = ']';
            } break;
            case '=': {
                read += 3;
                *write++ = '#';
            } break;
            case '/': {
                read += 3;
                *write++ = '\\';
            } break;
            case '\'': {
                read += 3;
                *write++ = '^';
            } break;
            case '!': {
                read += 3;
                *write++ = '|';
            } break;
            case '-': {
                read += 3;
                *write++ = '~';
            } break;
            }
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
    return write;
}

char *
remove_backslash_newline(char *p) {
    char *write = p;
    char *read  = p;
    uint32_t n  = 0;
    while (*read) {
        if (read[0] == '\\' && read[1] == '\n') {
            read += 2;
            ++n;
        } else if (*read == '\n') {
            *write++ = *read++;
            while (n--) {
                *write++ = '\n';
            }
            n = 0;
        } else {
            *write++ = *read++;
        }
    }

    while (n--) {
        *write++ = '\n';
    }

    *write = 0;
    return write;
}

static string
read_file_data(string filename, allocator *a) {
    FILE *file = fopen(filename.data, "r");
    assert(file);
    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = aalloc(a, size + 1);
    fread(data, 1, size, file);
    data[size] = 0;
    fclose(file);

    return string(data, size);
}

file *
get_file(file_storage *fs, string name) {
    file *file = 0;
    for (file = fs->files; file; file = file->next) {
        if (string_eq(file->name, name)) {
            break;
        }
    }

    if (!file) {
        file                = aalloc(fs->a, sizeof(file));
        file->name          = name;
        string contents     = read_file_data(name, fs->a);
        file->contents_init = contents;

        char *s = contents.data;
        // BOM
        if (strcmp(s, "\xef\xbb\xbf") == 0) {
            s += 3;
        }
        // Phase 1
        canonicalize_newline(s);
        replace_trigraphs(s);
        // Phase 2
        char *send     = remove_backslash_newline(s);
        file->contents = string(s, send - s);
    }
    return file;
}

void
report_error_internalv(char *filename, char *file_contents, uint32_t nline,
                       uint32_t ncol, char *fmt, va_list args) {
    char *line = file_contents;
    uint32_t line_counter = 0;
    while (line_counter < nline) {
        if (!*line) {
            NOT_IMPL;
            return;
        }

        if (*line == '\n') {
            ++line_counter;
        }
        ++line;
    }

    char *line_end = line;
    while (*line_end && *line_end != '\n') {
        ++line_end;
    }

    uint32_t loc = fprintf(stderr, "%s:%u/%u: ", filename, nline, ncol);
    fprintf(stderr, "%.*s\n", (int)(line_end - line), line);
    fprintf(stderr, "%*c", ncol + loc, ' ');
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void
report_error_internal(char *filename, char *file_contents, uint32_t line,
                      uint32_t col, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_error_internalv(filename, file_contents, line, col, fmt, args);
}

void
report_errorv(file *f, uint32_t line, uint32_t col, char *fmt, va_list args) {
    report_error_internalv(f->name.data, f->contents.data, line, col, fmt, args);
}

void
report_error(file *f, uint32_t line, uint32_t col, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_errorv(f, line, col, fmt, args);
}

