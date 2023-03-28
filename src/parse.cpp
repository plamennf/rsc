#include "general.h"
#include "os.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum Comparison_Type {
    COMPARISON_UNKNOWN,
    COMPARISON_EQUALS,
    COMPARISON_NOT_EQUALS,
};

enum Token_Type {
    TOKEN_UNKNOWN,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_ASTERISK,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_EQUALS,
    TOKEN_DOUBLE_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_COMMA,
    TOKEN_EOF,
};

struct Token {
    Token_Type type;

    int text_length;
    char *text;

    bool equals(char *match);
};

struct Tokenizer {
    char *at;
    int line_number;
    
    Tokenizer(char *at) : at(at) {}

    void eat_whitespace();

    Token get_token();
    bool require_token(Token &token, Token_Type type);

    void report_error(char *fmt, ...);
};

inline bool is_end_of_line(char c) {
    return (c == '\n') || (c == '\r');
}

inline bool is_whitespace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\v');
}

inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// TODO: Add support for 0x and 0b and floating-point numbers starting with a dot(.5235)
inline bool is_number(char c) {
    return (c >= '0' && c <= '9');
}

bool Token::equals(char *match) {
    int i;
    for (i = 0; i < text_length; i++) {
        if (match[i] == 0 || text[i] != match[i]) return false;
    }
    return match[i] == 0;
}

void Tokenizer::eat_whitespace() {
    for (;;) {
        if (at[0] != '\n') {
            at++;
            line_number++;
        }
        
        if (is_whitespace(at[0])) {
            at++;
        } else if (at[0] == '/' && at[1] == '/'
    }
}
