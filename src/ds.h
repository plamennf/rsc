#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "general.h"

inline Memory_Arena array_arena;

template <typename T>
struct Array {
    T *data = NULL;
    s64 count = 0;
    s64 allocated = 0;
    
    inline void reserve(s64 newSize) {
        if (newSize > allocated) {
            resize(newSize);
        }
    }
    
    inline ~Array() {
        /*
        if (data && !use_temporary_storage) {
            ::operator delete(data, allocated * sizeof(T));
        }
        */
    }
    
    inline void add(const T& value) {
        if (count >= allocated) {
            if (!allocated) {
                allocated = 16;
            }
            resize(allocated * 2);
        }
        
        data[count] = value;
        count++;
    }

    inline void add(T *values, s64 num_values) {
        for (s64 i = 0; i < num_values; i++) {
            add(values[i]);
        }
    }
    
    inline T *add() {
        if (count >= allocated) {
            if (!allocated) {
                allocated = 16;
            }
            resize(allocated * 2);
        }

        T tmp = {};
        data[count] = tmp;
        T *result = &data[count++];
        return result;
    }
    
    inline void resize(s64 newSize) {
        T *new_block = (T *)array_arena.push(newSize * sizeof(T));
        
        if (newSize < count) {
            count = newSize;
        }
        
        for (s64 i = 0; i < count; i++) {
            new_block[i] = data[i];
        }
        
        data = new_block;
        allocated = newSize;
    }

    inline void remove_nth(s64 index) {
        for (s64 i = index; i < count-1; i++) {
            data[i] = data[i+1];
        }
        count--;
    }

    inline s64 find(T const &value) {
        for (s64 i = 0; i < count; i++) {
            if (data[i] == value) {
                return i;
            }
        }
        return -1;
    }
    
    inline T *copy_to_array() {
        T *result = new T[count];
        memcpy(result, data, count * sizeof(T));
        return result;
    }
    
    inline const T &get(s64 index) const {
        assert(index >= 0);
        assert(index < count);
        
        return data[index];
    }

    inline T &get(s64 index) {
        assert(index >= 0);
        assert(index < count);
        
        return data[index];
    }
    
    inline const T &operator[](s64 index) const {
        assert(index >= 0);
        assert(index < count);
        
        return data[index];
    }
    
    inline T &operator[](s64 index) {
        assert(index >= 0);
        assert(index < count);
        
        return data[index];
    }
};

inline int hash(int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

inline int hash(char *str) {
    int hash = 5381;

    //extern int get_string_length(char *str);
    int len = (int)strlen(str);//get_string_length(str);
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

#pragma warning(push, 0)
inline int hash(void *p) {
    return (int)p;
}
#pragma warning(pop)

template <typename Key, typename Value>
struct Hash_Table {
    struct Bucket {
        Key key;
        Value value;
    };

    Bucket *buckets = nullptr;
    bool *occupancy_mask = nullptr;
    int allocated = 0;
    int count = 0;

    inline void grow() {
        const int HASH_TABLE_INITIAL_CAPACITY = 256;

        if (!buckets) {
            assert(allocated == 0);
            assert(count == 0);
            
            buckets = (Bucket *)calloc(HASH_TABLE_INITIAL_CAPACITY, sizeof(Bucket));
            occupancy_mask = (bool *)calloc(HASH_TABLE_INITIAL_CAPACITY, sizeof(bool));
            allocated = HASH_TABLE_INITIAL_CAPACITY;
            count = 0;
        } else {
            Hash_Table <Key, Value> new_hash_table = {
                (Bucket *)calloc(allocated * 2, sizeof(Bucket)),
                (bool *)calloc(allocated * 2, sizeof(bool)),
                allocated * 2,
                0,
            };

            memset(new_hash_table.buckets, 0, allocated * 2 * sizeof(Bucket));
            memset(new_hash_table.occupancy_mask, 0, allocated * 2 * sizeof(bool));

            for (int i = 0; i < count; i++) {
                if (occupancy_mask[i]) {
                    new_hash_table.add(buckets[i].key, buckets[i].value);
                }
            }

            free(buckets);
            free(occupancy_mask);

            *this = new_hash_table;
        }
    }

    inline void add(Key key, Value value) {
        if (count >= allocated) {
            grow();
        }

        auto hk = hash(key) & (allocated - 1);
        while (occupancy_mask[hk] && buckets[hk].key != key) {
            hk = (hk + 1) & (allocated - 1);
        }

        occupancy_mask[hk] = true;
        buckets[hk].key = key;
        buckets[hk].value = value;
        count++;
    }

    inline Value *get(Key key) {
        auto hk = hash(key) & (allocated - 1);
        for (int i = 0; i < allocated && occupancy_mask[hk] && buckets[hk].key != key; i++) {
            hk = (hk + 1) & (allocated - 1);
        }

        if (buckets && occupancy_mask[hk] && buckets[hk].key == key) {
            return &buckets[hk].value;
        } else {
            return nullptr;
        }
    }

    inline bool contains(Key key) {
        return get(key);
    }

    Value *operator[](Key key) {
        {
            Value *maybe_value = get(key);
            if (!maybe_value) {
                add(key, {});
            } else {
                return maybe_value;
            }
        }
        Value *maybe_value = get(key);
        assert(maybe_value);
        return maybe_value;
    }
};

template <typename Value>
struct String_Hash_Table {
    struct Bucket {
        char *key;
        Value value;
    };

    Bucket *buckets = nullptr;
    bool *occupancy_mask = nullptr;
    int allocated = 0;
    int count = 0;

    inline void grow() {
        const int HASH_TABLE_INITIAL_CAPACITY = 256;

        if (!buckets) {
            assert(allocated == 0);
            assert(count == 0);
            
            buckets = (Bucket *)calloc(HASH_TABLE_INITIAL_CAPACITY, sizeof(Bucket));
            occupancy_mask = (bool *)calloc(HASH_TABLE_INITIAL_CAPACITY, sizeof(bool));
            allocated = HASH_TABLE_INITIAL_CAPACITY;
            count = 0;
        } else {
            String_Hash_Table <Value> new_hash_table = {
                (Bucket *)calloc(allocated * 2, sizeof(Bucket)),
                (bool *)calloc(allocated * 2, sizeof(bool)),
                allocated * 2,
                0,
            };

            for (int i = 0; i < count; i++) {
                if (occupancy_mask[i]) {
                    new_hash_table.add(buckets[i].key, buckets[i].value);
                }
            }

            free(buckets);
            free(occupancy_mask);

            *this = new_hash_table;
        }
    }

    inline void add(char *key, Value value) {
        if (count >= allocated) {
            grow();
        }

        auto hk = hash(key) & (allocated - 1);
        while (occupancy_mask[hk] && !strings_match(buckets[hk].key, key)) {
            hk = (hk + 1) & (allocated - 1);
        }

        occupancy_mask[hk] = true;
        buckets[hk].key = strdup(key);
        buckets[hk].value = value;
        count++;
    }

    inline Value *get(char *key) {
        auto hk = hash(key) & (allocated - 1);
        for (int i = 0; i < allocated && occupancy_mask[hk] && strcmp(buckets[hk].key, key); i++) {
            hk = (hk + 1) & (allocated - 1);
        }

        if (buckets && occupancy_mask[hk] && !strcmp(buckets[hk].key, key)) {
            return &buckets[hk].value;
        } else {
            return nullptr;
        }
    }

    inline bool contains(char *key) {
        return get(key);
    }

    Value *operator[](char *key) {
        {
            Value *maybe_value = get(key);
            if (!maybe_value) {
                add(key, {});
            } else {
                return maybe_value;
            }
        }
        Value *maybe_value = get(key);
        assert(maybe_value);
        return maybe_value;
    }
};
