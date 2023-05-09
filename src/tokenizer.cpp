#include "tokenizer.h"

#include <stdio.h>
#include <stdarg.h>

bool Token::equals(char *match) {
    char *at = match;
    for (int i = 0; i < textLength; i++, at++) {
        if ((at[0] == 0) || text[i] != at[0]) {
            return false;
        }
    }

    return at[0] == 0;
}

void Tokenizer::reportError(char *fmt, ...) {
    char buf[4096];
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fprintf(stderr, "Error in line %d of file '%s': %s\n", lineNumber, filepath, buf);
}

void Tokenizer::eatWhitespace() {
    for (;;) {
        if (at[0] == '\n') {
            lineNumber++;
            at++;
        } else if (isWhitespace(at[0])) {
            at++;
        } else if (isWhitespace(at[0] == '#')) {
            at++;

            while (at[0] && !isEndOfLine(at[0])) {
                at++;
            }
        } else {
            break;
        }
    }
}

Token Tokenizer::getToken() {
    eatWhitespace();

    Token token = {};
    token.textLength = 1;
    token.text = at;

    switch (at[0]) {
    case '\0': token.type = TokenType_EOF; at++; break;
        
    case '(': token.type = TokenType_OpenBrace; at++; break;
    case ')': token.type = TokenType_CloseBrace; at++; break;
    case ':': token.type = TokenType_Colon; at++; break;
    case ';': token.type = TokenType_Semicolon; at++; break;
    case '*': token.type = TokenType_Asterisk; at++; break;
    case '[': token.type = TokenType_OpenBracket; at++; break;
    case ']': token.type = TokenType_CloseBracket; at++; break;
    case '{': token.type = TokenType_OpenBrace; at++; break;
    case '}': token.type = TokenType_CloseBrace; at++; break;
    case '=': token.type = TokenType_Equals; at++; break;
    case ',': token.type = TokenType_Comma; at++; break;

    case '"': {
        at++;
        token.text = at;
        
        while (at[0] && at[0] != '"') {
            if ((at[0] == '\\') && at[1]) {
                at++;
            }
            at++;
        }
        
        token.type = TokenType_String;
        token.textLength = (i32)(at - token.text);

        if (at[0] == '"') {
            at++;
        } else {
            reportError("A string should always end with a closing quote");
        }
    } break;

    default: {
        if (isAlpha(at[0]) || at[0] == '_') {
            while (isAlpha(at[0]) ||
                   isNumber(at[0]) ||
                   at[0] == '_') {
                at++;
            }

            token.type = TokenType_Identifier;
            token.textLength = (i32)(at - token.text);
        } else if (isNumber(at[0])) {
            while (isNumber(at[0]) && at[0]) {
                at++;
            }

            token.type = TokenType_Number;
            token.textLength = (i32)(at - token.text);
        } else {
            token.type = TokenType_Unknown;
        }
    } break;
    }

    return token;
}

char *toString(TokenType type) {
    switch (type) {
    case TokenType_EOF:          return "EOF";
    case TokenType_Colon:        return ":";
    case TokenType_Comma:        return ",";
    case TokenType_Semicolon:    return ";";
    case TokenType_Asterisk:     return "*";
    case TokenType_Equals:       return "=";
    case TokenType_OpenBrace:    return "{";
    case TokenType_CloseBrace:   return "}";
    case TokenType_OpenParen:    return "(";
    case TokenType_CloseParen:   return ")";
    case TokenType_OpenBracket:  return "[";
    case TokenType_CloseBracket: return "]";
    case TokenType_Identifier:   return "(identifier)";
    case TokenType_String:       return "(string)";
    case TokenType_Number:       return "(number)";
    }

    return "(unknown)";
}

bool Tokenizer::expectToken(Token *token, TokenType type) {
    Assert(token);
    *token = getToken();

    if (token->type != type) {
        char *article = "";
        if (type == TokenType_String || type == TokenType_Number) {
            article = "a ";
        } else if (type == TokenType_Identifier) {
            article = "an ";
        }
        reportError("Expected %s'%s', but instead found '%s'", article, toString(type), toString(token->type));
        return false;
    }

    return true;
}

bool Tokenizer::expectToken(Token *token, char *text) {
    Assert(token);
    *token = getToken();

    if (token->type != TokenType_Identifier) {
        reportError("Expected an identifier '%s', but instead found a token of type '%s'", text, toString(token->type));
        return false;
    }

    return true;
}
