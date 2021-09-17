#include "filesystem.h"

#include <stdio.h>

#include "strings.h"
#include "memory.h"
#include "hashing.h"

#include <sys/syslimits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

FileID create_file(const char *filename, u32 mode) {
    int posix_mode = 0;
    if (mode == FILE_MODE_READ) {
        posix_mode |= O_RDONLY;
    } else if (mode == FILE_MODE_WRTIE) {
        posix_mode |= O_WRONLY;
    } 
    int posix_handle = open(filename, posix_mode);
    FileID id = {0};
    if (posix_handle > 0) {
        id.value = posix_handle;
    }
    return id;
}
FileID get_stdout_file(void) {
    FileID id;
    id.value = 1;
    return id;
}
FileID get_stderr_file(void) {
    FileID id;
    id.value = 2;
    return id;
}
FileID get_stdin_file(void) {
    FileID id;
    id.value = 3;
    return id;
}

b32 write_file(FileID file, uptr offset, const void *bf, uptr bf_sz) {
    b32 result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file.value;
        if (offset != UINT32_MAX) {
            lseek(posix_handle, offset, SEEK_SET);
        }
        ssize_t written = write(posix_handle, bf, bf_sz);
        if (bf_sz == written) {
            result = 1;
        }
    }
    return result;
}
b32 read_file(FileID file, uptr offset, void *bf, uptr bf_sz) {
    b32 result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file.value;
        if (offset != UINT32_MAX) {
            lseek(posix_handle, offset, SEEK_SET);
        }
        ssize_t nread = read(posix_handle, bf, bf_sz);
        if (nread == bf_sz) {
            result = 1;
        }
    }
    return result;
}
uptr get_file_size(FileID id) {
    uptr result = 0;
    if (is_file_valid(id)) {
        int posix_handle = id.value;
        result = lseek(posix_handle, 0, SEEK_END);
    }      
    return result;
}
b32 is_file_valid(FileID id) {
    return id.value != 0;
}

// @SPEED it may be benefitial to use hash table for storing filenames,
// because latency of os calls is unpredictible
uptr fmt_filename(char *bf, uptr bf_sz, FileID id) {
    uptr result = 0;
    if (is_file_valid(id)) {
        int posix_fd = id.value;
        char internal_bf[PATH_MAX];
        if (fcntl(posix_fd, F_GETPATH, internal_bf) != -1) {
            str_cp(bf, bf_sz, internal_bf);
        }
    }
    return result;
}