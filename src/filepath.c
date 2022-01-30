#include "filepath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "darray.h"
#include "str.h"

void
get_current_dir(char *buf, uint32_t buf_size) {
    getcwd(buf, buf_size);
}

bool
path_is_dir(string path) {
    bool result;
    struct stat st;
    result = stat(path.data, &st) != -1;
    return result || !!(st.st_mode & S_IFDIR);
}

string
path_dirname(string path) {
    string result;
    string_find_result find_result = string_rfind(path, '/');
    if (!find_result.is_found) {
        result = WRAPZ(".");
    }
    result = string_substr(path, 0, find_result.idx);
    return result;
}

string
path_filename(string path) {
    string result = WRAPZ(".");

    while (string_endswith(path, WRAPZ("/"))) {
        path = string_substr(path, 0, path.len - 1);
    }

    if (path.len) {
        string_find_result find_result = string_rfind(path, '/');
        if (!find_result.is_found) {
            result = path;
        } else {
            result = string_substr(path, find_result.idx + 1, path.len);
        }
    }
    return result;
}

string
path_basename(string path) {
    string filename                = path_filename(path);
    string_find_result find_result = string_rfind(path, '.');
    string result;
    if (!find_result.is_found) {
        result = filename;
    } else {
        result = string_substr(filename, 0, find_result.idx);
    }
    return result;
}

string
path_to_absolute(string path, struct allocator *a) {
    string result;
    if (string_startswith(path, WRAPZ("/"))) {
        result = path;
    } else {
        char dir[4096] = "";
        get_current_dir(dir, sizeof(dir));
        result = string_memprintf(a, "%s/%.*s", dir, path.len, path.data);
    }
    return result;
}

string
path_clean(string path, struct allocator *a) {
    bool is_rooted = string_startswith(path, WRAPZ("/"));
    string *its    = NULL;  // da

    while (path.len) {
        string_find_result slash = string_find(path, '/');
        string elem;

        if (!slash.is_found) {
            elem     = path;
            path.len = 0;
        } else {
            elem = string_substr(path, 0, slash.idx);
            path = string_substr(path, slash.idx + 1, path.len);
        }

        if (!elem.len || string_eq(elem, WRAPZ("."))) {
            continue;
        }

        if (string_eq(elem, WRAPZ(".."))) {
            if (!da_size(its)) {
                if (is_rooted) {
                    continue;
                }
                da_push(its, WRAPZ(".."), a);
                continue;
            }

            if (string_eq(*da_last(its), WRAPZ(".."))) {
                da_push(its, WRAPZ(".."), a);
            } else {
                da_pop(its);
            }
        }
    }

    char buffer[4096];
    uint32_t size = 0;

    if (is_rooted) {
        size += snprintf(buffer, sizeof(buffer) - size, "/");
    }

    for (uint32_t i = 0; i < da_size(its); ++i) {
        size += snprintf(buffer, sizeof(buffer) - size, "%*s", its[i].len,
                         its[i].data);
        if (i != da_size(its) - 1) {
            size += snprintf(buffer, sizeof(buffer) - size, "/");
        }
    }

    da_free(its, a);
    return string_strdup(a, buffer);
}
