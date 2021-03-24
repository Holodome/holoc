#if !defined(MEM_HH)

#include "general.hh"

#if COMPILER_LLVM 
#define aligned_malloc_base(_size, _align) ::aligned_alloc(_size, _align)
#elif COMPILER_MSVC
#define aligned_malloc_base(_size, _align) ::_aligned_malloc(_size, _align)
#else 
#error !
#endif 

namespace Mem {
    void *alloc_base(size_t size) {
        if (!size) {
            return 0;
        }
        size_t padded_size = (size + 15) & ~15;
        return aligned_malloc_base(padded_size, 16);
    }
    
    void *alloc_clear(size_t size) {
        void *result = alloc_base(size);
        memset(result, 0, size);
        return result;
    }
    
    void *alloc(size_t size) {
#if 1
        return alloc_clear(size);
#else
        return alloc(size);
#endif 
    }
    
    void free(void *ptr) {
       ::free(ptr);
    }
    
    void *dup(void *ptr, size_t size) {
        void *result = alloc(size);
        memcpy(result, ptr, size);
        return result;
    }
}

void *operator new(size_t size) {
    return Mem::alloc(size);
}
void operator delete(void *p) noexcept {
    Mem::free(p);
}
void *operator new[](size_t size) {
    return Mem::alloc(size);
}
void operator delete[](void *p) noexcept {
    Mem::free(p);
}

#define MEM_HH 1
#endif
