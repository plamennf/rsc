#ifndef ARRAY_H
#define ARRAY_H

template<typename T>
struct Array {
    T* data = NULL;
    umm count = 0;
    umm allocated = 0;
    
    Array(umm newSize = 32) {
        data = NULL;
        count = 0;
        allocated = 0;
        if (newSize != 0) {
            resize(newSize);
        }
    }
    
    ~Array() {
        free(data);
    }

    void emplace() {
        T temp = {};
        add(temp);
    }
    
    void add(const T& value) {
        if (count >= allocated) {
            resize(allocated + allocated / 2);
        }
        
        data[count] = value;
        count++;
    }
    
    void add(const Array<T> &other) {
        for (umm i = 0; i < other.count; i++) {
            add(other[i]);
        }
    }
    
    void resize(umm newSize) {
        data = (T *)realloc(data, newSize * sizeof(T));
        allocated = newSize;
        if (allocated < count) {
            count = allocated;
        }
    }

    const T &get(umm index) const {
        assert(index < count);
        
        return data[index];
    }

    T &get(umm index) {
        assert(index < count);
        
        return data[index];
    }
    
    const T& operator[](umm index) const {
        assert(index < count);
        
        return data[index];
    }
    
    T& operator[](umm index) {
        assert(index < count);
        
        return data[index];
    }
};

#endif
