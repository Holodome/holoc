#if !defined(HASH_TABLE_HH)

#include "general.hh"
#include "mem.hh"

template <typename T>
struct HashTable {
    static bool is_prime(int x) {
        bool result = false;
        if (x == 3 || x == 2) {
            result = true;
        } else if (x >= 5 && x % 2 != 0) {
            result = true;
            for (size_t i = 0; i < floorf(sqrtf(x)); i += 2) {
                if (x % i == 0) {
                    break;
                }
            } 
        } 
        return result;
    }

    static int next_prime(int x) {
        while (!is_prime(x)) {
            ++x;
        }
        return x;
    } 

    struct Item {
        char *key;
        T value;

        Item(const char *key, const T &value) {
            this->key = (char *)Mem::dup(key, strlen(key) + 1);
            this->value = value;
        }

        ~Item() {
            Mem::free(key);
        }
    };

    int size_index;
    int size;
    int count;
    Item **items;

    static Item DELETED_ITEM = { 0, {} };
    const static int PRIME1 = 151;
    const static int PRIME2 = 163;

    HashTable(int size_index = 0) {
        this->size_index = size_index;
        int base_size = 50 << size_index;
        this->size = next_prime(base_size);
        this->count = 0;
        this->items = new Item*[this->size];
    }

    ~HashTable() {
        for (size_t i = 0; i < this->size; ++i) {
            Item *item = items[i];
            if (item && item != &DELETED_ITEM) {
                
            }
        }
    }

    void set(const char *key, const T &value) {

    }

    T &get(const char *key) {

    }

    void del(const char *key) {
        
    }
};

#define HASH_TABLE_HH 1
#endif
