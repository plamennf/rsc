#ifndef CORE_H
#define CORE_H

#if _WIN32
#define OS_WINDOWS
#endif

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef unsigned long long umm;
typedef signed long long   smm;

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

typedef double             float64;
typedef float              float32;
typedef double             f64;
typedef float              f32;

// Copy-paste from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda) : lambda(lambda){}
    ~ExitScope(){lambda();}
    ExitScope(const ExitScope&);
private:
    ExitScope& operator =(const ExitScope&);
};
 
class ExitScopeHelp {
public:
    template<typename T>
    ExitScope<T> operator+(T t){ return t;}
};
 
#define defer const auto& CONCAT(defer__, __COUNTER__) = ExitScopeHelp() + [&]()

#define array_count(array) (sizeof(array)/sizeof((array)[0]))

inline umm get_string_length(char *a) {
    umm length = 0;
    while (*a) {
        ++length;
        ++a;
    }
    return length;
}

inline bool strings_match(char *a, char *b) {
    if (a == b) return true;
    if (get_string_length(a) != get_string_length(b)) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    if (a && !b) return false;
    if (b && !a) return false;
    return true;
}

inline char *copy_string(char *str) {
    umm length = get_string_length(str);
    char *result = new char[length + 1];
    memset(result, 0, length + 1);
    memcpy(result, str, length);
    return result;
}

inline void clamp(float *value, float min, float max) {
    if (!value) return;
    
    if (*value < min) *value = min;
    else if (*value > max) *value = max;
}

inline void clamp(int *value, int min, int max) {
    if (!value) return;

    if (*value < min) *value = min;
    else if (*value > max) *value = max;
}

inline char *find_character_from_right(char *s, char c) {
    return strrchr(s, c);
}

inline char *find_character_from_left(char *s, char c) {
    return strchr(s, c);
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
            if (*end == '\n') ++end;
        }

        *t = '\0';
    }
    
    *text_ptr = end;
    
    return s;
}

inline char *eat_spaces(char *at) {
    if (!at) return NULL;
    
    char *result = at;
    while (*result == ' ') {
        result++;
    }
    return result;
}

inline char *eat_trailing_spaces(char *at) {
    if (!at) return NULL;

    umm orig_len = get_string_length(at);
    umm len = 0;
    while (at[orig_len - len] == ' ') {
        len++;
    }

    char *result = at;
    result[orig_len - len] = 0;
    
    return result;
}

inline char *break_by_spaces(char **text_ptr) {
    char *t = *text_ptr;
    if (!*t) return NULL;

    char *s = t;

    char *end = t;
    if (*t) {
        while (*end && *end != ' ') {
            end++;
            t++;
        }

        if (*end == ' ') {
            end++;
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
    umm len = get_string_length(text);
    char *result = text;
    for (umm i = 0; i < len; ++i) {
        result[i] = uppercase(text[i]);
    }
    return result;
}

inline char *concatenate(char *a, char *b) {
    umm a_len = get_string_length(a);
    umm b_len = get_string_length(b);
    char *result = new char[a_len + b_len + 1];
    memcpy(result, a, a_len);
    memcpy(result + a_len, b, b_len);
    result[a_len + b_len] = 0;
    return result;
}

#endif
