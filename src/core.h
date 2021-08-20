#ifndef CORE_H
#define CORE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#if _WIN32
#define OS_WINDOWS 1
#endif

#ifndef NULL
#define NULL 0
#endif

#define var auto

typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;

typedef signed long long   s64;
typedef signed int         s32;
typedef signed short       s16;
typedef signed char        s8;

typedef signed long long   b64;
typedef signed int         b32;
typedef signed short       b16;
typedef signed char        b8;

typedef double             f64;
typedef float              f32;

void *operator new(size_t size) {
    if (size == 0) ++size;

    if (void *ptr = malloc(size)) {
        memset(ptr, 0, size);
        return ptr;
    }

    return NULL;
}

void operator delete(void *ptr) {
    free(ptr);
}

template <typename F>
struct saucy_defer {
	F f;
	saucy_defer(F f) : f(f) {}
	~saucy_defer() { f(); }
};

template <typename F>
saucy_defer<F> defer_func(F f) {
	return saucy_defer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) =     defer_func([&](){code;})

char *read_entire_file(FILE *file) {
    char *result = NULL;
    if (file) {
        fseek(file, 0, SEEK_END);
        var length = ftell(file);
        fseek(file, 0, SEEK_SET);

        result = new char[length + 1];
        fread(result, sizeof(char), length, file);
        fclose(file);
    }
    return result;
}

char *read_entire_file(char *file_path) {
    FILE *file = fopen(file_path, "r");
    return read_entire_file(file);
}

#define assert(expr) if(expr){}else{__debugbreak();}

template<typename T>
struct Array {
    T* data = NULL;
    size_t count = 0;
    size_t allocated = 0;
    
    Array(size_t newSize = 4) {
        resize(newSize);
    }
    
    ~Array() {
        free(data);
    }
    
    void add(const T& value) {
        if (count >= allocated) {
            resize(allocated + allocated / 2);
        }
        
        data[count] = value;
        count++;
    }
    
    void add(T&& value) {
        if (count >= allocated) {
            resize(allocated + allocated / 2);
        }
        
        data[count] = (T&&)value;
        count++;
    }
    
    void resize(size_t newSize) {
        data = (T *)realloc(data, newSize * sizeof(T));
        allocated = newSize;
        if (allocated < count) {
            count = allocated;
        }
    }

    const T &get(size_t index) const {
        assert(index < count);
        
        return data[index];
    }

    T &get(size_t index) {
        assert(index < count);
        
        return data[index];
    }
    
    const T& operator[](size_t index) const {
        assert(index < count);
        
        return data[index];
    }
    
    T& operator[](size_t index) {
        assert(index < count);
        
        return data[index];
    }
};

int round_float_to_int(float value) {
    return (int)(value + 0.5f);
}

float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

inline char *consume_next_line(char **text_ptr) {
    char *t = *text_ptr;
    if (!*t) return NULL;

    char *s = t;

    while (*t && (*t != '\n') && (*t != '\r')) t++;

    char *end = t;
    if (*t) {
        end++;

        if (*t == '\r') {
            if (*end == '\n') end++;
        }

        *t = '\0';
    }

    *text_ptr = end;

    return s;
}

inline char uppercase(char c) {
    char result = c;
    if (result >= 65 && result <= 90) return result;
    else result -= 32;
    return result;
}

inline char *to_upper(char *text) {
    size_t len = strlen(text);
    char *result = text;
    for (size_t i = 0; i < len; ++i) {
        result[i] = uppercase(text[i]);
    }
    return result;
}

#endif
