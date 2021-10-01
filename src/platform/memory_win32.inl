#include "general.h"
#include "memory.h"

#include <windows.h>

MemoryBlock *os_alloc_block(uptr size, u32 flags) {
    // @TODO(hl): There probably is little point in page alignment and stuff, because allocation requst size differs. There should be api of getting size for allocation outside of platform, so numbers here don't have to be tweaked 
    uptr page_size = 4096;
    // @NOTE: size must account for alignment, so when 
    uptr request_size = align_forward_pow2(size + sizeof(MemoryBlock), MEM_DEFAULT_ALIGNMENT);
    uptr request_size_page_aligned = align_forward_pow2(request_size, page_size);
    
    uptr total_size;
    uptr usable_size;
    uptr base_offset;
    uptr protect_offset = 0;
    if (flags & MEM_ALLOC_OVERFLOW_CHECK) {
        // Padded memory
        // Requested memory aligned 
        // Protected page
        total_size = request_size_page_aligned + page_size;
        usable_size = size;
        base_offset = align_backward_pow2(request_size_page_aligned - size, MEM_DEFAULT_ALIGNMENT);
        protect_offset = request_size_page_aligned;
    } else if (flags & MEM_ALLOC_UNDERFLOW_CHECK) {
        // Page 1: MemoryBlock
        // Page 2: Protected
        // Pages 2+: Memory
        // @NOTE this layout is so verbose because memory block should be in head of everything
        total_size = request_size_page_aligned + 2 * page_size;
        usable_size = request_size_page_aligned;
        base_offset = 2 * page_size;
        protect_offset = page_size;
    } else {
        total_size = request_size_page_aligned;
        usable_size = total_size - sizeof(MemoryBlock);
        base_offset = sizeof(MemoryBlock);
    }
    assert(usable_size >= size);
    assert((total_size & (page_size - 1)) == 0);
    
    LPVOID memory = VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory);
    
    MemoryBlock *block = (MemoryBlock *)memory;
    block->size = usable_size;
    block->base = (u8 *)memory + base_offset;
    if (flags & (MEM_ALLOC_OVERFLOW_CHECK | MEM_ALLOC_UNDERFLOW_CHECK)) {
        DWORD old_protect = 0;
        BOOL is_valid = VirtualProtect((u8 *)memory + protect_offset, page_size, PAGE_NOACCESS, &old_protect);
        assert(is_valid);
    }
    
    return block;
}

#error Not finished!!!
#endif 