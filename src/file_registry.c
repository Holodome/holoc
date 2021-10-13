#include "file_registry.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "lib/files.h"

File_Registry *
create_file_registry(Memory_Arena *arena) {
    File_Registry *fr = arena_alloc_struct(arena, File_Registry);
    fr->arena = arena;
    fr->hash_table.num_buckets = MAX_FILES;
    fr->hash_table.keys = arena_alloc_array(arena, MAX_FILES, u64);
    fr->hash_table.values = arena_alloc_array(arena, MAX_FILES, u64);
    return fr;
}

File_ID 
register_file(File_Registry *fr, const char *filename) {
    File_ID result = {0};
    u32 filename_length = str_len(filename);
    u64 filename_hash = fnv64(filename, filename_length);
    result.opaque = filename_hash;
    
    Hash_Table64_Get_Result get_result = hash_table64_get(&fr->hash_table, filename_hash);
    if (get_result.is_valid) {
        // nop     
    } else {
        assert(fr->next_file_idx < MAX_FILES);
        u32 value_idx = fr->next_file_idx++;
        File_Registry_Entry *file = arena_alloc_struct(fr->arena, File_Registry_Entry);
        file->id = filename_hash;
        assert(filename_length + 1 < MAX_FILEPATH_LENGTH);
        mem_copy(file->path, filename, filename_length + 1);
        fr->files[value_idx] = file;
        
        hash_table64_set(&fr->hash_table, filename_hash, value_idx);
    }
    
    return result;    
}

File_Data_Get_Result
get_file_data(File_Registry *fr, File_ID id) {
    File_Data_Get_Result result = {0};
    
    assert(id.opaque);
    Hash_Table64_Get_Result get_result = hash_table64_get(&fr->hash_table, id.opaque);
    if (get_result.is_valid) {
        u32 file_idx = get_result.value;
        File_Registry_Entry *file = fr->files[file_idx];
        assert(file->id == id.opaque);
        if (!file->has_data) {
            OS_File_Handle file_handle = os_open_file_read(file->path);
            if (OS_IS_FILE_HANDLE_VALID(file_handle)) {
                u64 file_size = os_get_file_size(file_handle);
                void *data = arena_alloc(fr->arena, file_size + 1);
                os_read_file(file_handle, 0, data, file_size);
                ((char *)data)[file_size] = 0;
                file->data.data = data;
                file->data.data_size = file_size;
                os_close_file(file_handle);
                result.is_valid = true;
            } else {
                // @TODO logging
                NOT_IMPLEMENTED;
            }
        } else {
            result.is_valid = true;
        } 
        result.data = file->data;
    } 
    return result;
}

const char *
get_file_path(File_Registry *fr, File_ID id) {
    const char *result = 0;
    
    assert(id.opaque);
    Hash_Table64_Get_Result get_result = hash_table64_get(&fr->hash_table, id.opaque);
    if (get_result.is_valid) {
        u32 file_idx = get_result.value;
        File_Registry_Entry *file = fr->files[file_idx];
        assert(file->id == id.opaque);
        result = file->path;
    } 
    return result;
}
