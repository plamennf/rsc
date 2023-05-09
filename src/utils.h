#pragma once

#include "defines.h"
#include "dynamic_array.h"

i64 getStringLength(char *s);
char *copyString(char *s);
char *copyStringLowercased(char *s);
char *copyStripExtension(char *filename);
bool stringsMatch(char *a, char *b);

bool isEndOfLine(char c);
bool isWhitespace(char c);
bool isAlpha(char c);
bool isNumber(char c);

char *consumeNextLine(char **textPointer);
char *eatWhitespace(char *s);
bool startsWith(char *a, char *b);

char *mprintf(char *fmt, ...);

struct StringBuilder {
    DynamicArray<char> buffer;

    void copyFrom(StringBuilder *other);
    
    void printf(char *fmt, ...);

    void add(char *s);
    void add(char c);
    
    char *toString();
};
