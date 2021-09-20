#include "filesystem.h"

#include <stdio.h>

#include "strings.h"
#include "memory.h"
#include "hashing.h"

#include <sys/syslimits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

FileHandle open_file(const char *filename, u32 mode) {
    int posix_mode = 0;
    if (mode == FILE_MODE_READ) {
        posix_mode |= O_RDONLY;
    } else if (mode == FILE_MODE_WRITE) {
        posix_mode |= O_WRONLY | O_TRUNC | O_CREAT;
    } 
    int permissions = 0777;
    int posix_handle = open(filename, posix_mode, permissions);
    FileHandle file = {0};
    if (posix_handle > 0) {
        file.value = posix_handle;
    } else if (posix_handle == -1) {
        file.flags |= FILE_FLAG_HAS_ERRORS;
    }
    return file;
}

b32 close_file(FileHandle *id) {
    b32 result = close(id->value) == 0;
    if (result) {
        id->flags |= FILE_FLAG_IS_CLOSED;
    }
    return result;
}

FileHandle *get_stdout_file(void) {
    static FileHandle id = { 1, FILE_FLAG_IS_ST };
    return &id;
}
FileHandle *get_stderr_file(void) {
    static FileHandle id = { 2, FILE_FLAG_IS_ST };
    return &id;
}
FileHandle *get_stdin_file(void) {
    static FileHandle id = { 3, FILE_FLAG_IS_ST };
    return &id;
}

uptr write_file(FileHandle *file, uptr offset, const void *bf, uptr bf_sz) {
    uptr result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file->value;
        if (offset != 0xFFFFFFFF) {
            lseek(posix_handle, offset, SEEK_SET);
        }
        ssize_t written = write(posix_handle, bf, bf_sz);
        if (written == -1) {
            DBG_BREAKPOINT;
        }
        result = written;
    } else {
        DBG_BREAKPOINT;
    }
    return result;
}

uptr read_file(FileHandle *file, uptr offset, void *bf, uptr bf_sz) {
    uptr result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file->value;
        if (offset != 0xFFFFFFFF) {
            lseek(posix_handle, offset, SEEK_SET);
        }
        ssize_t nread = read(posix_handle, bf, bf_sz);
        if (nread == -1) {
            DBG_BREAKPOINT;
        }
        result = nread;
    } else {
        DBG_BREAKPOINT;
    }
    return result;
}
uptr get_file_size(FileHandle *id) {
    uptr result = 0;
    if (is_file_valid(id)) {
        int posix_handle = id->value;
        result = lseek(posix_handle, 0, SEEK_END);
    }      
    return result;
}
b32 is_file_valid(FileHandle *id) {
    return id->value != 0 && !(id->flags & FILE_FLAG_NOT_OPERATABLE);
}

// @SPEED it may be benefitial to use hash table for storing filenames,
// because latency of os calls is unpredictible
uptr fmt_filename(char *bf, uptr bf_sz, FileHandle *id) {
    uptr result = 0;
    if (is_file_valid(id)) {
        int posix_fd = id->value;
        char internal_bf[PATH_MAX];
        if (fcntl(posix_fd, F_GETPATH, internal_bf) != -1) {
            str_cp(bf, bf_sz, internal_bf);
        }
    }
    return result;
}