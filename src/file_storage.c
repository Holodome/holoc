#include "file_storage.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "darray.h"
#include "filepath.h"
#include "str.h"

void
add_default_include_paths(file_storage *fs) {
    allocator *a  = get_system_allocator();
    string *paths = 0;
    // Linux folders
    da_push(paths, WRAP_Z("/usr/local/include"), a);
    da_push(paths, WRAP_Z("/usr/include/x86_64-linux-gnu"), a);
    da_push(paths, WRAP_Z("/usr/include"), a);
    // HL: This is copy from my mac clang paths
    da_push(paths,
            WRAP_Z("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                   "XcodeDefault.xctoolchain/usr/lib/clang/13.0.0/include"),
            a);
    da_push(paths,
            WRAP_Z("/Applications/Xcode.app/Contents/Developer/Platforms/"
                   "MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"),
            a);
    da_push(paths,
            WRAP_Z("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                   "XcodeDefault.xctoolchain/usr/include"),
            a);
    da_push(paths,
            WRAP_Z("/Applications/Xcode.app/Contents/Developer/Platforms/"
                   "MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/"
                   "Frameworks"),
            a);

    fs->include_paths      = paths;
    fs->include_path_count = da_size(paths);
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
        result = string_memdup(fs->a, buffer);
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
            result = string_memdup(fs->a, buffer);
        }
    }
    return result;
}

file *
get_file(file_storage *fs, string name, string current_name) {
    file *file = 0;
    for (file = fs->files; file; file = file->next) {
        if (string_eq(file->name, name)) {
            break;
        }
    }

    if (!file) {
        string actual_path = {0};
        if (current_name.data) {
            actual_path = get_filepath_in_same_dir(fs, name, current_name);
        }
        if (!actual_path.data) {
            actual_path = get_filepath_from_include_paths(fs, name);
        }
        if (!actual_path.data) {
            NOT_IMPL;
        }

        file                = aalloc(fs->a, sizeof(file));
        file->name          = string_dup(fs->a, name);
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
    char *line            = file_contents;
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
    report_error_internalv(f->name.data, f->contents.data, line, col, fmt,
                           args);
}

void
report_error(file *f, uint32_t line, uint32_t col, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_errorv(f, line, col, fmt, args);
}

