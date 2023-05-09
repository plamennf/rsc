#pragma once

#include "defines.h"

#include <stdlib.h>
#include <string.h>

template <typename T>
struct DynamicArray {
    int capacity;
    int count;
    T *data;

    inline DynamicArray() : capacity(0), count(0), data(NULL) {}
    inline ~DynamicArray() { if (data) free(data); }

    inline void reserve(int wantedSize) {
        if (capacity >= wantedSize) return;

        int newCapacity = wantedSize > capacity * 2 ? wantedSize : capacity * 2;
        newCapacity = newCapacity > 32 ? newCapacity : 32;

        T *newData = (T *)malloc(newCapacity * sizeof(T));
        if (data) {
            memcpy(newData, data, count * sizeof(T));
            free(data);
        }

        data = newData;
        capacity = newCapacity;
    }

    inline void resize(int wantedSize) {
        reserve(wantedSize);
        count = wantedSize;
    }
    
    inline void add(T const &item) {
        reserve(count+1);
        data[count] = item;
        count++;
    }

    inline T *add() {
        T tmp = {};
        add(tmp);
        return &data[count - 1];
    }

    inline T *copyToArray() {
        T *result = (T *)malloc(count * sizeof(T));
        memcpy(result, data, count * sizeof(T));
        return result;
    }

    inline T const &operator[](int index) const {
        Assert(index >= 0);
        Assert(index < count);
        return data[index];
    }

    inline T &operator[](int index) {
        Assert(index >= 0);
        Assert(index < count);
        return data[index];
    }
};
