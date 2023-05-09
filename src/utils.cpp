#include "utils.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

char *toCString(char *text, i32 textLength);

i64 getStringLength(char *s) {
    if (!s) return 0;

    i64 length = 0;
    while (*s++) {
        length++;
    }
    return length;
}

char *copyString(char *s) {
    i64 length = getStringLength(s);
    char *result = new char[length + 1];
    memcpy(result, s, length + 1);
    return result;
}

char *copyStringLowercased(char *s) {
    i64 length = getStringLength(s);
    char *result = new char[length + 1];
    result[length] = 0;
    
    for (i64 i = 0; i < length; i++) {
        s[i] = tolower(s[i]);
    }
    
    return result;
}

char *copyStripExtension(char *filename) {
    if (!filename) return NULL;

    char *result = copyString(filename);
    char *dot = strrchr(result, '.');
    result[dot - result] = 0;
    return result;
}

bool stringsMatch(char *a, char *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    while (*a && *b) {
        if (*a != *b) {
            return false;
        }

        a++;
        b++;
    }

    return (a[0] == 0) && (b[0] == 0);
}

bool isEndOfLine(char c) {
    return ((c == '\n') || (c == '\r'));
}

bool isWhitespace(char c) {
    return ((c == ' ') ||
            (c == '\t') ||
            (c == '\n') ||
            (c == '\r') ||
            (c == '\v'));
}

bool isAlpha(char c) {
    return (((c >= 'a') && (c <= 'z')) ||
            ((c >= 'A') && (c <= 'Z')));
}

bool isNumber(char c) {
    return ((c >= '0') && (c <= '9'));
}

char *consumeNextLine(char **textPointer) {
    char *t = *textPointer;
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
    
    *textPointer = end;
    
    return s;
}

char *eatWhitespace(char *s) {
    if (!s) return NULL;

    while (isWhitespace(s[0])) {
        s++;
    }
    return s;
}

bool startsWith(char *a, char *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    i64 aLen = getStringLength(a);
    i64 bLen = getStringLength(b);
    if (aLen < bLen) return false;
    
    for (i64 i = 0; i < bLen; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

char *mprintf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = 1 + vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *str = (char *)malloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *mprintf_valist(char *fmt, va_list args) {
    va_list ap = args;
    size_t n = 1 + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *str = (char *)malloc(n);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

void StringBuilder::copyFrom(StringBuilder *other) {
    buffer.data = other->buffer.copyToArray();
    buffer.count = other->buffer.count;
    buffer.capacity = other->buffer.count;
}

void StringBuilder::printf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *buf = mprintf_valist(fmt, args);
    va_end(args);
    defer { free(buf); };

    for (char *at = buf; *at; at++) {
        buffer.add(at[0]);
    }
}

void StringBuilder::add(char *s) {
    for (char *at = s; *at; at++) {
        buffer.add(at[0]);
    }
}

void StringBuilder::add(char c) {
    buffer.add(c);
}

char *StringBuilder::toString() {
    return toCString(buffer.data, buffer.count);
}
