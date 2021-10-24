#define WIN32_LEAN_AND_MEAN
#include <windows.h>

OS_File_Handle 
os_open_file_read(const char *filename) {
    OS_File_Handle handle = {0};
    
    HANDLE win32_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, 0, 0);
    if (win32_handle) {
        handle.opaque = *(u64 *)&win32_handle;
        handle.flags |= FILE_HAS_NO_ERRORS_BIT;
    } 
    // @TODO(hl): Errors
    return handle;
}


OS_File_Handle 
os_open_file_write(const char *filename) {
    OS_File_Handle handle = {0};
    
    HANDLE win32_handle = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE,
        0, CREATE_ALWAYS, 0, 0);
    if (win32_handle) {
        handle.opaque = *(u64 *)&win32_handle;
        handle.flags |= FILE_HAS_NO_ERRORS_BIT;
    } 
    // @TODO(hl): Errors
    return handle;
}

bool 
os_close_file(OS_File_Handle handle) {
    bool result = CloseHandle(*((HANDLE *)handle.opaque));
    return result;
}

uptr 
os_write_file(OS_File_Handle file, uptr offset, const void *bf, uptr bf_sz) {
    u64 written = 0;
    if (OS_IS_FILE_HANDLE_VALID(file)) {
        HANDLE win32handle = *((HANDLE *)file.opaque);
        OVERLAPPED overlapped = {};
        overlapped.Offset     = (u32)((offset >> 0)  & 0xFFFFFFFF);
        overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
        
        u32 size32 = (u32)bf_sz;
        assert(size32 == bf_sz);
        
        DWORD bytes_wrote;
        if (WriteFile(win32handle, bf, size32, &bytes_wrote, &overlapped) &&
            (size32 == bytes_wrote)) {
            written = bytes_wrote;
            // Success        
        } 
    }    
    return written;
}

uptr 
os_read_file(OS_File_Handle file, uptr offset, void *bf, uptr bf_sz) {
    u64 written = 0;
    if (OS_IS_FILE_HANDLE_VALID(file)) {
        HANDLE win32handle = *((HANDLE *)file.opaque);
        OVERLAPPED overlapped = {};
        overlapped.Offset     = (u32)((offset >> 0)  & 0xFFFFFFFF);
        overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
        
        u32 size32 = (u32)bf_sz;
        assert(size32 == bf_sz);
        
        DWORD bytes_read;
        if (ReadFile(win32handle, bf, size32, &bytes_read, &overlapped) &&
            (size32 == bytes_read)){
            written = bytes_read;
            // Success        
        } 
    }
    return written;
}

uptr 
os_write_stdout(void *bf, uptr bf_sz) {
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), bf, bf_sz, 0, 0);
    return bf_sz;
}

uptr 
os_write_stderr(void *bf, uptr bf_sz) {
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), bf, bf_sz, 0, 0);
    return bf_sz;
}

u64 
os_get_file_size(OS_File_Handle id) {
    DWORD result = GetFileSize(*((HANDLE *)id.opaque), 0);
    return (uptr)result;
}

OS_Stat 
os_stat(const char *filename) {
    OS_Stat result = {0};
    
    WIN32_FILE_ATTRIBUTE_DATA win32_attr = {0};
    bool got = GetFileAttributesExA(filename, GetFileExInfoStandard, &win32_attr);
    if (got) {
        result.exists = true;
        result.is_directory = TO_BOOL(win32_attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        result.size = (u64)win32_attr.nFileSizeHigh << 32 | win32_attr.nFileSizeLow;
    }
    return result;    
}