#if !defined(ARRAY_HH)

#include "mem.hh"

template <typename T>
void *array_new(size_t capacity) {
    T *ptr = (T *)Mem::alloc(capacity * sizeof(T));
    for (u32 i = 0; i < capacity; ++i) {
        ptr[i] = T();
    }
    return ptr;
}

template <typename T>
void array_delete(void *ptr, size_t capacity) {
    for (u32 i = 0; i < capacity; ++i) {
        ((T *)ptr)[i].~T();
    }
    Mem::free(ptr);
}

template <typename T>
void *array_resize(void *old_ptrv, size_t old_capacity, size_t new_capacity) {
    T *old_ptr = (T *)old_ptrv;
    T *new_ptr = 0;
    if (new_capacity) {
        new_ptr = (T *)array_new<T>(new_capacity);
        size_t overlap = old_capacity;
        if (new_capacity < old_capacity) {
            overlap = new_capacity;
        }
        for (size_t i = 0; i < overlap; ++i) {
            new_ptr[i] = old_ptr[i];
        }
    }
    array_delete<T>(old_ptr, old_capacity);
    return new_ptr;
}

template <typename T> 
struct Array {
    T *data;
    size_t len, capacity;
    
    Array() {
        data = 0;
        clear();
    }
    ~Array() {
        array_delete<T>(data, capacity);
    }
    
    void clear() {
        if (data) {
            array_delete<T>(data, capacity);
        }
        data = 0;
        capacity = 0;
        len = 0;
    }
    
    void resize(size_t count) {
        if (!count) {
            clear();
            return;
        }    
        if (count == capacity) {
            return;
        }
        data = (T *)array_resize<T>(data, capacity, count);
        capacity = count;
        if (capacity < len) {
            len = capacity;
        }
    }
    
    size_t add(const T &obj) {
        if (len + 1 > capacity) {
            // @TODO currently we double the size of array but later we may want to increment it instead
            size_t new_capacity = capacity * 2;
            if (new_capacity == 0) {
                new_capacity = 16;
            }
            resize(new_capacity);
        }
        
        data[len] = obj;
        return len++;
    }
    
    const T &operator[](size_t idx) const {
        assert(idx < len);
        return data[idx];
    }
    
    T &operator[](size_t idx) {
        assert(idx < len);
        return data[idx];
    }
};

// Temporary array that is deleted when goes out of skope
template <typename T>
struct TempArray {
    T *data;
    size_t len;
    
    TempArray() = delete;
    TempArray(size_t size_init) {
        data = (T *)Mem::alloc(size_init * sizeof(T));
        len = size_init;
    }
    
    ~TempArray() {
        Mem::free(data);
    }
  
    T &operator [](size_t idx) {
        assert(idx < len);
        return data[idx];
    }  
    
    const T &operator [](size_t idx) const {
        assert(idx < len);
        return data[idx];
    }  
    
    size_t mem_size() const {
        return sizeof(T) * len;
    }
};

#define ARRAY_HH 1
#endif
