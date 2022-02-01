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

static file_storage fs_;
static file_storage *fs = &fs_;

file_storage *
get_file_storage(void) {
    return fs;
}

void
fs_add_default_include_paths(void) {
    // Linux folders
    da_push(fs->include_paths, WRAPZ("/usr/local/include"), fs->a);
    da_push(fs->include_paths, WRAPZ("/usr/include/x86_64-linux-gnu"), fs->a);
    da_push(fs->include_paths, WRAPZ("/usr/include"), fs->a);
    // HL: This is copy from my mac clang paths
    da_push(fs->include_paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                  "XcodeDefault.xctoolchain/usr/lib/clang/13.0.0/include"),
            fs->a);
    da_push(fs->include_paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Platforms/"
                  "MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"),
            fs->a);
    da_push(fs->include_paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                  "XcodeDefault.xctoolchain/usr/include"),
            fs->a);
    da_push(fs->include_paths,
            WRAPZ("/Applications/Xcode.app/Contents/Developer/Platforms/"
                  "MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/"
                  "Frameworks"),
            fs->a);
}

void
fs_add_include_paths(string *new_paths, uint32_t new_path_count) {
    for (uint32_t i = 0; i < new_path_count; ++i) {
        da_push(fs->include_paths, new_paths[i], fs->a);
    }
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
get_filepath_in_same_dir(string name, string current_path) {
    string result      = {0};
    string current_dir = path_dirname(current_path);
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "%.*s/%.*s", current_dir.len,
             current_dir.data, name.len, name.data);
    if (file_exists(buffer)) {
        result = string_strdup(fs->a, buffer);
    }
    return result;
}

static string
get_filepath_from_include_paths(string name) {
    string result = {0};
    for (uint32_t i = 0; i < da_size(fs->include_paths); ++i) {
        string dir = fs->include_paths[i];
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), "%.*s/%.*s", dir.len, dir.data,
                 name.len, name.data);
        if (file_exists(buffer)) {
            result = string_strdup(fs->a, buffer);
        }
    }
    return result;
}

static string
get_filepath_relative(string name) {
    string result = {0};
    char buffer[4096];
    get_current_dir(buffer, sizeof(buffer));
    uint32_t dir_len = strlen(buffer);
    snprintf(buffer + dir_len, sizeof(buffer) - dir_len, "/%.*s", name.len,
             name.data);
    if (file_exists(buffer)) {
        result = string_strdup(fs->a, buffer);
    }
    return result;
}

file *
fs_get_file(string name, file *current_file) {
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
                get_filepath_in_same_dir(name, current_file->full_path);
        }
        if (!actual_path.data) {
            actual_path = get_filepath_from_include_paths(name);
        }
        if (!actual_path.data) {
            actual_path = get_filepath_relative(name);
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

