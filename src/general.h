#pragma once

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#if _MSC_VER
#include <intrin.h>
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

typedef double   float64;
typedef float    float32;

// Copy-paste from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// Defer macro/thing.

#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda):lambda(lambda){}
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
 
#define defer const auto& CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define ArrayCount(array) (sizeof(array)/sizeof((array)[0]))
#define Square(x) ((x)*(x))
#define Min(x, y) ((x) < (y) ? (x) : (y))
#define Max(x, y) ((x) > (y) ? (x) : (y))
#define Clamp(x, a, b) { if ((x) < (a)) (x) = (a); if ((x) > (b)) (x) = (b); }

#define Kilobytes(x) ((x)*1024ULL)
#define Megabytes(x) (Kilobytes(x)*1024ULL)
#define Gigabytes(x) (Megabytes(x)*1024ULL)
#define Terabytes(x) (Gigabytes(x)*1024ULL)

const float PI = 3.14159265359f;
const float TAU = 6.28318530718f;

inline int absolute_value(int value) {
    if (value < 0) return -value;
    return value;
}

inline float absolute_value(float value) {
    float result = fabsf(value);
    return result;
}

inline float square_root(float value) {
    float result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
    return result;
}

inline s32 round_float32_to_s32(float value) {
    s32 result = _mm_cvtss_si32(_mm_set_ss(value));
    return result;
}

inline s32 floor_float32_to_s32(float value) {
    s32 result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(value)));
    return result;
}

struct Memory_Arena {
    s64 size = 0;
    s64 occupied = 0;
    u8 *data = 0;

    void init(s64 size);
    void *push(s64 wanted_size);
};

void init_temporary_storage(s64 size);
void reset_temporary_storage();
s64 get_temporary_storage_mark();
void set_temporary_storage_mark(s64 mark);
void *talloc(s64 mark, s64 alignment = 8);

char *sprint(char *fmt, ...);
char *sprint_valist(char *fmt, va_list args);
char *tprint(char *fmt, ...);
char *tprint_valist(char *fmt, va_list args);

int get_string_length(char *s);
char *copy_string(char *s, bool use_temporary_storage = false);
bool strings_match(char *a, char *b);
int get_codepoint(char *text, int *bytes_processed);

char *eat_spaces(char *s);
bool starts_with(char *string, char *substring);
char *consume_next_line(char **text_ptr);

float fract(float value);

char *temp_concatenate_with_commas(char **array, int array_count);

void log(char *string, ...);
void log_error(char *string, ...);
void logprint(char *agent, char *string, ...);

char *find_character_from_left(char *string, char c);
char *eat_trailing_spaces(char *string);
bool contains(char *string, char *substring);

char *replace_backslash_with_forwardslash(char *string);
char *replace_forwardslash_with_backslash(char *string);

char *lowercase(char *string);
char *temp_copy_strip_extension(char *filename);
