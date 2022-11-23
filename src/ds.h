#pragma once

#include "general.h"

#include <stdlib.h>
#include <string.h>

template <typename Array>
struct Array_Iterator {
    using Value_Type = typename Array::Value_Type;
    using Pointer_Type = Value_Type *;
    using Reference_Type = Value_Type &;
    
    inline Array_Iterator(Pointer_Type ptr) {
        this->ptr = ptr;
    }

    inline Array_Iterator &operator++() {
        ptr++;
        return *this;
    }
    
    inline Array_Iterator operator++(int) {
        Array_Iterator iterator = *this;
        ++(*this);
        return iterator;
    }
    
    inline Array_Iterator &operator--() {
        ptr--;
        return *this;
    }
    
    inline Array_Iterator operator--(int) {
        Array_Iterator iterator = *this;
        --(*this);
        return iterator;        
    }

    inline Reference_Type operator[](int index) {
        return *(ptr + index);
    }
    
    inline Pointer_Type operator->() {
        return ptr;
    }
    
    inline Reference_Type operator*() {
        return *ptr;
    }

    inline bool operator==(Array_Iterator const &other) const {
        return ptr == other.ptr;
    }
    
    inline bool operator!=(Array_Iterator const &other) const {
        return !(*this == other);
    }
    
    Pointer_Type ptr;
};

template <typename T>
struct Array {
    using Value_Type = T;
    using Iterator = Array_Iterator<Array<T>>;
    
    T *data = NULL;
    int allocated = 0;
    int count = 0;

    ~Array();

    void reserve(int size);
    void resize(int size);
    void add(T const &item);
    int find(T const &item);
    void ordered_remove_by_index(int n);

    inline void add(T *items, int item_count) {
        reserve(count + item_count);
        for (int i = count; i < count + item_count; i++) {
            data[i] = items[i - count];
        }
        count += item_count;
    }

    inline T *copy_to_array() {
        T *result = new T[count];
        memcpy(result, data, count * sizeof(T));
        return result;
    }
    
    T const &operator[](int index) const;
    T &operator[](int index);

    inline Iterator begin() { return data; }
    inline Iterator end() { return data + count; }
};

template <typename T>
inline Array <T>::~Array() {
    if (data) {
        free(data);
    }
}

template <typename T>
inline void Array <T>::reserve(int size) {
    if (allocated >= size) return;

    int new_allocated = Max(allocated, size);
    new_allocated = Max(new_allocated, 32);

    int new_bytes = new_allocated * sizeof(T);
    int old_bytes = allocated * sizeof(T);

    void *new_data = malloc(new_bytes);
    if (data) {
        memcpy(new_data, data, old_bytes);
    }

    if (data) {
        free(data);
    }

    data = (T *)new_data;

    allocated = new_allocated;
}

template <typename T>
inline void Array <T>::resize(int size) {
    reserve(size);
    count = size;
}

template <typename T>
inline void Array <T>::add(T const &item) {
    reserve(count+1);
    data[count] = item;
    count++;
}

template <typename T>
inline int Array <T>::find(T const &item) {
    for (int i = 0; i < count; i++) {
        if (data[i] == item) return i;
    }
    return -1;
}

template <typename T>
inline void Array <T>::ordered_remove_by_index(int n) {
    for (int i = n; i < count-1; i++) {
        data[i] = data[i+1];
    }
    count--;
}

template <typename T>
inline T const &Array <T>::operator[](int index) const {
    assert(index >= 0);
    assert(index < count);
    return data[index];
}

template <typename T>
inline T &Array <T>::operator[](int index) {
    assert(index >= 0);
    assert(index < count);
    return data[index];    
}

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
