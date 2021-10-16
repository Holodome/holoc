#include "file_registry.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "lib/files.h"
#include "lib/path.h"

#include "compiler_ctx.h"

File_Registry *
create_file_registry(Compiler_Ctx *ctx) {
    File_Registry *fr = arena_alloc_struct(ctx->arena, File_Registry);
    fr->ctx = ctx;
    fr->hash_table.num_buckets = MAX_FILES;
    fr->hash_table.keys = arena_alloc_array(ctx->arena, MAX_FILES, u64);
    fr->hash_table.values = arena_alloc_array(ctx->arena, MAX_FILES, u64);
    return fr;
}

static u32
fmt_path_for_file(char *buffer, u32 buffer_size, File_Registry *fr, const char *filename) {
    u32 result = 0;
    for (u32 search_path_idx = 0;
         search_path_idx < fr->include_seach_paths_count; 
         ++search_path_idx) {
        const char *include_search_path = fr->include_search_paths[search_path_idx];
             
        char temp_file_buffer[MAX_FILEPATH_LENGTH];
        fmt(temp_file_buffer, sizeof(temp_file_buffer), "%s/%s",
            include_search_path, filename);
        
        OS_Stat temp_stat = os_stat(temp_file_buffer);
        if (temp_stat.exists) {
            result = zcp(buffer, buffer_size, temp_file_buffer);
            break;
        }
    }
    return result;
}

static u32 
fmt_path_in_same_dir(char *buffer, u32 buffer_size, File_Registry *fr, const char *filename, File_ID friend_id) {
    u32 result = 0;
    Hash_Table64_Get_Result friend_get = hash_table64_get(&fr->hash_table, friend_id.opaque);
    assert(friend_get.is_valid);
    File_Registry_Entry *friend = fr->files[friend_get.value];
    assert(friend->id = friend_id.opaque);
    Str friend_path = strz(friend->path);
    Str base_path = path_parent(friend_path);
    char temp_buffer[MAX_FILEPATH_LENGTH];
    fmt(temp_buffer, sizeof(temp_buffer), "%.*s/%s", 
        base_path.len, base_path.data, filename);
    OS_Stat temp_stat = os_stat(temp_buffer);
    if (temp_stat.exists) {
        result = zcp(buffer, buffer_size, temp_buffer);
    }
    
    return result;
}

File_ID 
register_file(File_Registry *fr, const char *filename, File_ID current_file) {
    File_ID result = {0};
    u32 filename_length = zlen(filename);
    u64 filename_hash = fnv64(filename, filename_length);
    result.opaque = filename_hash;
    
    Hash_Table64_Get_Result get_result = hash_table64_get(&fr->hash_table, filename_hash);
    if (get_result.is_valid) {
        // nop     
    } else {
        assert(fr->next_file_idx < MAX_FILES);
        u32 value_idx = fr->next_file_idx++;
        File_Registry_Entry *file = arena_alloc_struct(fr->ctx->arena, File_Registry_Entry);
        file->id = filename_hash;
        
        zcp(file->path_init, sizeof(file->path_init), filename);
        if (current_file.opaque) {
            fmt_path_in_same_dir(file->path, sizeof(file->path), fr, 
                filename, current_file);
        } 
        
        if (!*file->path) {
            fmt_path_for_file(file->path, sizeof(file->path), fr, filename);
            
            if (!*file->path) {
                OS_Stat file_stat = os_stat(filename);
                if (file_stat.exists) {
                    zcp(file->path, sizeof(file->path), filename);
                } else {
                    NOT_IMPLEMENTED;
                }
            }
        }
        
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
                void *data = arena_alloc(fr->ctx->arena, file_size + 1);
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
