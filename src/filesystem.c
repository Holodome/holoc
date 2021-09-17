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

FileID create_file(const char *filename, u32 mode) {
    int posix_mode = 0;
    if (mode == FILE_MODE_READ) {
        posix_mode |= O_RDONLY;
    } else if (mode == FILE_MODE_WRITE) {
        posix_mode |= O_WRONLY | O_TRUNC | O_CREAT;
    } 
    int permissions = 0777;
    int posix_handle = open(filename, posix_mode, permissions);
    FileID id = {0};
    if (posix_handle > 0) {
        id.value = posix_handle;
    } else {
        outf("ERRNO: %d\n", errno);
        DBG_BREAKPOINT;
    }
    return id;
}

b32 destroy_file(FileID id) {
    b32 result = close(id.value) == 0;
    return result;
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

uptr write_file(FileID file, uptr offset, const void *bf, uptr bf_sz) {
    uptr result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file.value;
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

uptr read_file(FileID file, uptr offset, void *bf, uptr bf_sz) {
    uptr result = 0;
    if (is_file_valid(file)) {
        int posix_handle = file.value;
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