#pragma once

#include "utils.h"

enum TokenType {
    TokenType_Unknown,
    TokenType_EOF,
    TokenType_Colon,
    TokenType_Comma,
    TokenType_Semicolon,
    TokenType_Asterisk,
    TokenType_Equals,
    TokenType_OpenBrace,
    TokenType_CloseBrace,
    TokenType_OpenParen,
    TokenType_CloseParen,
    TokenType_OpenBracket,
    TokenType_CloseBracket,
    TokenType_Identifier,
    TokenType_String,
    TokenType_Number,
};

struct Token {
    TokenType type;

    i32 textLength;
    char *text;

    bool equals(char *match);
};

struct Tokenizer {
    char *filepath;
    char *at;
    int lineNumber;
    
    Tokenizer(char *filepath, char *at) : filepath(filepath), at(at), lineNumber(1) {}

    void eatWhitespace();
    Token getToken();
    bool expectToken(Token *token, TokenType type);
    bool expectToken(Token *token, char *text);
    
    void reportError(char *fmt, ...);
};

char *toString(TokenType type);
