#include "file_registry.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "lib/files.h"
#include "lib/path.h"
#include "lib/hashing.h"
#include "lib/lists.h"

#include "compiler_ctx.h"

static void 
prepare_source_file(File_Registry_Entry *file) {
    const char *cursor = file->data.data;
}

File_Registry *
create_file_registry(Compiler_Ctx *ctx) {
    File_Registry *fr = arena_alloc_struct(ctx->arena, File_Registry);
    fr->ctx = ctx;
    fr->hash_table.num_buckets = MAX_FILES;
    fr->hash_table.keys = arena_alloc_arr(ctx->arena, MAX_FILES, u64);
    fr->hash_table.values = arena_alloc_arr(ctx->arena, MAX_FILES, u64);
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
                file->data.size = file_size;
                os_close_file(file_handle);
                result.is_valid = true;
                prepare_source_file(file);
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

static u32 
get_src_loc_hash_internal(const Src_Loc *loc, u32 crc) {
    if (loc) {
        crc ^= loc->hash_value;
        crc = get_src_loc_hash_internal(loc->parent, crc);
    }
    return crc;
}

static u32 
get_src_loc_hash(const Src_Loc *loc) {
    u32 parent_crc = get_src_loc_hash_internal(loc->parent, 0);
    u32 current_crc = crc32(parent_crc, loc, sizeof(*loc));
    return current_crc;
}

Src_Loc **
get_src_loc_internal(File_Registry *fr, u32 hash) {
    CT_ASSERT(IS_POW2(FILE_REGISTRY_SRC_LOC_HASH_SIZE));
    u32 hash_value = hash & FILE_REGISTRY_SRC_LOC_HASH_SIZE;
    Src_Loc **loc = fr->src_loc_hash + hash_value;
    while (*loc && ((*loc)->hash_value != hash_value)) {
        loc = &(*loc)->next;
    }
    
    return loc;
}

static void 
copy_src_loc(Src_Loc *dest, Src_Loc *src) {
    Src_Loc *next = dest->next;
    mem_copy(dest, src, sizeof(*dest));
    dest->next = next;
}

static Src_Loc *
add_src_loc_internal(File_Registry *fr, Src_Loc *source) {
    Src_Loc *result = 0;
    u32 loc_hash = get_src_loc_hash(source);
    Src_Loc **loc = get_src_loc_internal(fr, loc_hash);
    if (!*loc) {
        Src_Loc *new_loc = arena_alloc_struct(fr->ctx->arena, Src_Loc);
        copy_src_loc(new_loc, source);
        new_loc->hash_value = loc_hash;
        STACK_ADD(*(loc), new_loc);
        result = new_loc;
    } else {
        result = *loc;
    }
    return result;
}

const Src_Loc *
get_src_loc_file(File_Registry *fr, File_ID id, u32 line, u32 symb, const Src_Loc *parent) {
    Src_Loc new_src_loc = {0};
    new_src_loc.kind = SRC_LOC_FILE;
    new_src_loc.file = id;
    new_src_loc.line = line;
    new_src_loc.symb = symb;
    new_src_loc.parent = parent;
    return add_src_loc_internal(fr, &new_src_loc);
}

const Src_Loc *
get_src_loc_macro(File_Registry *fr, Src_Loc *declared_at, u32 symb, const Src_Loc *parent) {
    Src_Loc new_src_loc = {0};
    new_src_loc.kind = SRC_LOC_MACRO;
    new_src_loc.declared_at = declared_at;
    new_src_loc.symb = symb;
    new_src_loc.parent = parent;
    return add_src_loc_internal(fr, &new_src_loc);
}

const Src_Loc *
get_src_loc_macro_arg(File_Registry *fr, u32 symb, const Src_Loc *parent) {
    Src_Loc new_src_loc = {0};
    new_src_loc.kind = SRC_LOC_MACRO_ARG;
    new_src_loc.symb = symb;
    new_src_loc.parent = parent;
    return add_src_loc_internal(fr, &new_src_loc);
}