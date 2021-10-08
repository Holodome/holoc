#include "lib/files.h"

#include <stdio.h>

#include "lib/strings.h"
#include "lib/memory.h"
#include "lib/hashing.h"

#include <sys/syslimits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>



OS_File_Handle 
os_open_file_read(const char *filename) {
    OS_File_Handle handle = {0};
    
    int posix_mode = O_RDONLY;
    int permissions = 0500;
    int posix_handle = open(filename, posix_mode, permissions);
    if (posix_handle > 0) {
        handle.opaque = posix_handle;
        handle.flags |= FILE_HAS_NO_ERRORS_BIT;
    } 
    // @TODO(hl): Errors
    return handle;
}


OS_File_Handle 
os_open_file_write(const char *filename) {
    OS_File_Handle handle = {0};

    int posix_mode = O_WRONLY | O_TRUNC | O_CREAT;
    int permissions = 0500;
    int posix_handle = open(filename, posix_mode, permissions);
    if (posix_handle > 0) {
        handle.opaque = posix_handle;
        handle.flags |= FILE_HAS_NO_ERRORS_BIT;
    } 
    // @TODO(hl): Errors

    return handle;
}

bool 
os_close_file(OS_File_Handle handle) {
    bool result = close(handle.opaque) == 0;
    return result;
}

uptr 
os_write_file(OS_File_Handle file, uptr offset, const void *bf, uptr bf_sz) {
    uptr result = 0;
    if (OS_IS_FILE_HANDLE_VALID(file)) {
        int posix_handle = file.opaque;
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
os_read_file(OS_File_Handle file, uptr offset, void *bf, uptr bf_sz) {
    uptr result = 0;
    if (OS_IS_FILE_HANDLE_VALID(file)) {
        int posix_handle = file.opaque;
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
os_get_file_size(OS_File_Handle id) {
    uptr result = 0;
    if (OS_IS_FILE_HANDLE_VALID(id)) {
        int posix_handle = id.opaque;
        result = lseek(posix_handle, 0, SEEK_END);
    }      
    return result;
}
