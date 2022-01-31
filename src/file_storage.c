#include "file_storage.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "darray.h"
#include "filepath.h"
#include "llist.h"
#include "str.h"
#include "unicode.h"

void
add_default_include_paths(string **pathsp) {
    allocator *a  = get_system_allocator();
    string *paths = *pathsp;
    // Linux folders
    da_push(paths, WRAPZ("/usr/local/include"), a);
    da_push(paths, WRAPZ("/usr/include/x86_64-linux-gnu"), a);
    da_push(paths, WRAPZ("/usr/include"), a);
    // HL: This is copy from my mac clang paths
    da_push(paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                  "XcodeDefault.xctoolchain/usr/lib/clang/13.0.0/include"),
            a);
    da_push(paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Platforms/"
                  "MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"),
            a);
    da_push(paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                  "XcodeDefault.xctoolchain/usr/include"),
            a);
    da_push(paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Platforms/"
                  "MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/"
                  "Frameworks"),
            a);
    *pathsp = paths;
}

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

static bool
file_exists(char *filename) {
    bool result = false;
    FILE *f     = fopen(filename, "rb");
    if (f) {
        fclose(f);
        result = true;
    }
    return result;
}

static string
get_filepath_in_same_dir(file_storage *fs, string name, string current_path) {
    string result      = {0};
    string current_dir = path_dirname(current_path);
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "%.*s/%s", current_dir.len,
             current_dir.data, name.data);
    if (file_exists(buffer)) {
        result = string_strdup(fs->a, buffer);
    }
    return result;
}

static string
get_filepath_from_include_paths(file_storage *fs, string name) {
    string result = {0};
    for (uint32_t i = 0; i < fs->include_path_count; ++i) {
        string dir = fs->include_paths[i];
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), "%s/%s", dir.data, name.data);
        if (file_exists(buffer)) {
            result = string_strdup(fs->a, buffer);
        }
    }
    return result;
}

static string
get_filepath_relative(file_storage *fs, string name) {
    string result = {0};
    char buffer[4096];
    get_current_dir(buffer, sizeof(buffer));
    uint32_t dir_len = strlen(buffer);
    snprintf(buffer + dir_len, sizeof(buffer) - dir_len, "/%s", name.data);
    if (file_exists(buffer)) {
        result = string_strdup(fs->a, buffer);
    }
    return result;
}

file *
get_file(file_storage *fs, string name, file *current_file) {
    file *f = NULL;
    for (f = fs->files; f; f = f->next) {
        if (string_eq(f->full_path, name)) {
            break;
        }
    }

    if (!f) {
        string actual_path = {0};
        if (current_file) {
            actual_path =
                get_filepath_in_same_dir(fs, name, current_file->full_path);
        }
        if (!actual_path.data) {
            actual_path = get_filepath_from_include_paths(fs, name);
        }
        if (!actual_path.data) {
            actual_path = get_filepath_relative(fs, name);
        }

        if (actual_path.data) {
            assert(file_exists(actual_path.data));

            f                = aalloc(fs->a, sizeof(file));
            f->name          = string_dup(fs->a, name);
            f->full_path     = actual_path;
            string contents  = read_file_data(actual_path, fs->a);
            f->contents_init = contents;

            char *s = contents.data;
            // BOM
            if (strcmp(s, "\xef\xbb\xbf") == 0) {
                s += 3;
            }
            // Phase 1
            canonicalize_newline(s);
            replace_trigraphs(s);
            // Phase 2
            char *send  = remove_backslash_newline(s);
            f->contents = string(s, send - s);

            LLIST_ADD(fs->files, f);
        }
    }
    return f;
}

void
report_error_internalv(char *filename, char *file_contents, uint32_t nline,
                       uint32_t ncol, char *fmt, va_list args) {
    char *line            = file_contents;
    uint32_t line_counter = 1;
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

    uint32_t utf8_coln = 0;
    char *cursor       = line;
    while (cursor < line + ncol) {
        uint32_t cp;
        cursor = utf8_decode(cursor, &cp);
        ++utf8_coln;
    }

    char *line_end = line + 1;
    while (*line_end && *line_end != '\n') {
        ++line_end;
    }

    fprintf(stderr, "033[1m%s:%u:%u: 033[31merror:033[0;1m ", filename, nline,
            utf8_coln);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "033[0m\n");
    fprintf(stderr, "%.*s\n", (int)(line_end - line), line);
    if (utf8_coln != 1) {
        fprintf(stderr, "%*c", utf8_coln - 1, ' ');
    }
    fprintf(stderr, "^\n");
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
    report_error_internalv(f->name.data, f->contents.data, line, col, fmt,
                           args);
}

void
report_error(file *f, uint32_t line, uint32_t col, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_errorv(f, line, col, fmt, args);
}

