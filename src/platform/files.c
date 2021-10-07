#include "platform/files.h"

#include <stdio.h>

#include "lib/strings.h"
#include "platform/memory.h"
#include "lib/hashing.h"

#include <sys/syslimits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void 
os_open_file(OS_File_Handle *file, const char *filename, u32 mode) {
    file->flags = 0;
    file->handle = 0;
    
    int posix_mode = 0;
    if (mode == FILE_MODE_READ) {
        posix_mode |= O_RDONLY;
    } else if (mode == FILE_MODE_WRITE) {
        posix_mode |= O_WRONLY | O_TRUNC | O_CREAT;
    } 
    int permissions = 0500;
    int posix_handle = open(filename, posix_mode, permissions);
    if (posix_handle > 0) {
        file->handle = posix_handle;
    } else if (posix_handle == -1) {
        file->flags |= FILE_FLAG_HAS_ERRORS;
        erroutf("Errno: %d", errno);
        DBG_BREAKPOINT;
    }
}

bool 
os_close_file(OS_File_Handle *id) {
    bool result = close(id->handle) == 0;
    if (result) {
        id->flags |= FILE_FLAG_IS_CLOSED;
    }
    return result;
}

uptr 
os_write_file(OS_File_Handle *file, uptr offset, const void *bf, uptr bf_sz) {
    uptr result = 0;
    if (os_is_file_valid(file)) {
        int posix_handle = file->handle;
        lseek(posix_handle, offset, SEEK_SET);
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

uptr 
os_read_file(OS_File_Handle *file, uptr offset, void *bf, uptr bf_sz) {
    uptr result = 0;
    if (os_is_file_valid(file)) {
        int posix_handle = file->handle;
        lseek(posix_handle, offset, SEEK_SET);
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

uptr 
os_write_stdout(void *bf, uptr bf_sz) {
    return write(1, bf, bf_sz);
}

uptr 
os_write_stderr(void *bf, uptr bf_sz) {
    return write(2, bf, bf_sz);
}

uptr 
os_get_file_size(OS_File_Handle *id) {
    uptr result = 0;
    if (os_is_file_valid(id)) {
        int posix_handle = id->handle;
        result = lseek(posix_handle, 0, SEEK_END);
    }      
    return result;
}

bool 
os_is_file_valid(OS_File_Handle *id) {
    return id->handle != 0 && !(id->flags & FILE_FLAG_NOT_OPERATABLE);
}