#include "path.h"

Str 
path_extension(Str path) {
    Str result = {0};
    u32 last_dot_idx = str_rfindc(path, '.');
    if (last_dot_idx != (u32)-1) {
        result = str_substr(path, last_dot_idx, path.len);
    }
    return result;
}

Str 
path_strip_extension(Str path) {
    Str result = {0};
    u32 last_dot_idx = str_rfindc(path, '.');
    if (last_dot_idx != (u32)-1) {
        result = str_substr(path, 0, last_dot_idx);
    }
    return result;
}

Str 
path_parent(Str path) {
    Str result = {0};
    path = str_rstrip(path, WRAP_Z("/"));
    if (path.len == 0) {
        result = WRAP_Z(".");
    } else if (str_countc(path, '/') == path.len) {
        result = WRAP_Z("/");
    } else {
        u32 last_slash_idx = str_rfind(path, WRAP_Z("/"));
        if (last_slash_idx != UINT32_MAX) {
            result = str_substr(path, 0, last_slash_idx);
        } else {
            result = path;
        }
    }
    return result;
}

Str 
path_dir(Str path) {
    Str result = {0};
    NOT_IMPLEMENTED;
    return result;
}

const char *
path_extensionz(const char *path) {
    NOT_IMPLEMENTED;
    return 0;
}

const char *
path_basez(const char *path) {
    NOT_IMPLEMENTED;
    return 0;
}