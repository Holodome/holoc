#include "filesystem.h"
#include "memory.h"
#include "hashing.h"
#include "error_reporter.h"
#include "strings.h"

#define FS_HASH_SIZE 128

typedef struct FSFileSlot {
    u64 hash;
    char *name; // @TODO Filepath should be here
    b32 is_open;
    u32 file_mode;
    uptr file_size_cached;
    OSFileHandle handle;
} FSFileSlot;

typedef struct FSFilepathSlot {
    u64 hash;
    Filepath filepath;
    char *rel_path_cached;
    char *abs_path_cached;
} FSFilepathSlot;

typedef struct {
    MemoryArena arena;
    
    Hash64 file_hash;
    u64 nfile_slots;
    u64 nfile_slots_used;
    FSFileSlot *file_hash_slots;
    // @TODO Free list for ids
    
    Hash64 filepath_hash;
    u64 nfilepath_slots;
    u64 nfilepath_slots_used;
    FSFilepathSlot *filepath_hash_slots;
} FS;

static FS *fs;

void init_filesystem(void) {
    fs = arena_bootstrap(FS, arena);
    fs->nfile_slots = FS_HASH_SIZE;
    fs->file_hash = create_hash64(fs->nfile_slots, &fs->arena);
    fs->file_hash_slots = arena_alloc_array(&fs->arena, fs->nfile_slots, FSFileSlot);
    fs->nfilepath_slots = FS_HASH_SIZE;
    fs->filepath_hash = create_hash64(fs->nfilepath_slots, &fs->arena);
    fs->filepath_hash_slots = arena_alloc_array(&fs->arena, fs->nfilepath_slots, FSFilepathSlot);
    fs->nfile_slots_used++;
    fs->nfilepath_slots_used++;
}

b32 is_file_id_valid(FileID id) {
    return id.value != 0;
}

static FSFileSlot *get_slot(u64 hash) {
    FSFileSlot *slot = 0;
    u64 default_value = (u64)-1;
    u64 idx = hash64_get(&fs->file_hash, hash, default_value);
    if (idx != default_value) {
        assert(idx < FS_HASH_SIZE);
        slot = fs->file_hash_slots + idx;
    }
    return slot;
}

static u64 get_hash_for_filename(const char *filename) {
    // @TOOD Use filepaths
    return hash_string(filename);
}

static u64 get_new_file_slot_idx(void) {
    assert(fs->nfile_slots_used < fs->nfile_slots);
    return fs->nfile_slots_used++;
}

static void open_slot_file(FSFileSlot *slot) {
    if (!slot->is_open) {
        os_open_file(&slot->handle, slot->name, slot->file_mode);
        slot->is_open = TRUE;
    }
}

FileID fs_get_id_for_filename(const char *filename) {
    FileID result = {0};
    // @TODO Construct filepath
    u64 filename_hash = get_hash_for_filename(filename);
    FSFileSlot *slot = get_slot(filename_hash);
    if (slot && slot->hash) {
        result.value = slot->hash;
    }
    return result;
}

OSFileHandle *fs_get_handle(FileID id) {
    OSFileHandle *handle = 0;
    // @TODO Construct filepath
    FSFileSlot *slot = get_slot(id.value);
    if (slot && slot->hash) {
        handle = &slot->handle;
    }
    return handle;
}

FileID fs_open_file(const char *name, u32 mode) {
    FileID result = {0};
    // Check if it already open
    u64 hash = get_hash_for_filename(name);
    FSFileSlot *slot = get_slot(hash);
    if (slot) {
        report_error_general("File is already open: '%s'", name);
    } else {
        u64 new_slot_idx = get_new_file_slot_idx();
        if (new_slot_idx != (u64)-1) {
            hash64_set(&fs->file_hash, hash, new_slot_idx);
            slot = fs->file_hash_slots + new_slot_idx;
            slot->hash = hash;
            // @LEAK
            slot->name = arena_alloc_str(&fs->arena, name);
            slot->file_mode = mode;
            open_slot_file(slot);
            slot->file_size_cached = (u64)-1;
            result.value = hash;
        }
    }
    
    return result;    
}

b32 fs_close_file(FileID id) {
    b32 result = FALSE;
    FSFileSlot *slot = get_slot(id.value);
    if (!slot) {
        char bf[1024] = {0};
        // fs_fmt_filename(bf, sizeof(bf), id);
        report_error_general("No file open for file id %llu (%s)", id.value, bf);
    } else {
        os_close_file(&slot->handle);
        slot->is_open = FALSE;
        hash64_set(&fs->file_hash, slot->hash, -1);
        slot->hash = 0;
        // @TODO Think about policy for closed files - do we want to have some of their contents
        // cached, shoul we free the slot or leave it for information...
    }
    return result;
}

uptr fs_get_file_size(FileID id) {
    uptr result = 0;
    FSFileSlot *slot = get_slot(id.value);
    if (slot) {
        if (slot->file_size_cached == (u64)-1) {
            slot->file_size_cached = os_get_file_size(&slot->handle);
        }
        result = slot->file_size_cached;
    }
    return result;    
}

uptr fs_fmt_filename(char *bf, uptr bf_sz, FileID id) {
    uptr result = 0;
    FSFileSlot *slot = get_slot(id.value);
    if (slot) {
        result = str_cp(bf, bf_sz, slot->name);
    }
    return result;    
}