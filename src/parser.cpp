#include "main.h"
#include "utils.h"
#include "tokenizer.h"

#include <stdlib.h>
#include <string.h>

char *toCString(char *text, i32 textLength) {
    char *result = (char *)malloc(textLength + 1);
    memcpy(result, text, textLength);
    result[textLength] = 0;
    return result;
}

static char *toCString(Token token) {
    return toCString(token.text, token.textLength);
}

static bool parseStringArray(Tokenizer *tokenizer, DynamicArray<char *> &strArr) {
    Token token;
    tokenizer->expectToken(&token, TokenType_Equals);
    tokenizer->expectToken(&token, TokenType_OpenBrace);
    
    bool firstRun = true;
    for (;;) {
        token = tokenizer->getToken();
        if (token.type == TokenType_CloseBrace) break;

        if (!firstRun) {
            if (token.type != TokenType_Comma) {
                tokenizer->reportError("Expected ',' after string");
                return false;
            }

            token = tokenizer->getToken();
            if (token.type == TokenType_CloseBrace) break;
            else if (token.type != TokenType_String) {
                tokenizer->reportError("Expected a string");
                return false;
            }
        }

        strArr.add(toCString(token));
        firstRun = false;
    }

    tokenizer->expectToken(&token, TokenType_Semicolon);

    return true;
}

static bool parseIdentifierArray(Tokenizer *tokenizer, DynamicArray<char *> &identArr) {
    Token token;
    tokenizer->expectToken(&token, TokenType_Equals);
    tokenizer->expectToken(&token, TokenType_OpenBrace);
    
    bool firstRun = true;
    for (;;) {
        token = tokenizer->getToken();
        if (token.type == TokenType_CloseBrace) break;

        if (!firstRun) {
            if (token.type != TokenType_Comma) {
                tokenizer->reportError("Expected ',' after identifier");
                return false;
            }

            token = tokenizer->getToken();
            if (token.type == TokenType_CloseBrace) break;
            else if (token.type != TokenType_Identifier) {
                tokenizer->reportError("Expected an identifier");
                return false;
            }
        }

        identArr.add(toCString(token));
        firstRun = false;
    }

    tokenizer->expectToken(&token, TokenType_Semicolon);

    return true;
}

static bool parseProject(Tokenizer *tokenizer, RscProject *project) {
    Token token;
    if (!tokenizer->expectToken(&token, TokenType_String)) return false;
    project->name = toCString(token);
    if (!tokenizer->expectToken(&token, TokenType_OpenBrace)) return false;

    RscConfiguration *currentConfiguration = NULL;
    
    for (;;) {
        token = tokenizer->getToken();
        if (token.type == TokenType_CloseBrace) {
            if (currentConfiguration) {
                currentConfiguration = NULL;
                continue;
            } else {
                break;
            }
        }
        
        if (token.type != TokenType_Identifier) {
            tokenizer->reportError("Expected an '(identifier)', but instead found '%s'", toString(token.type));
            return false;
        }
        
        if (token.equals("kind")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;

            OutputKind kind = OutputKind_ConsoleApp;
            if (token.equals("ConsoleApp")) {
                kind = OutputKind_ConsoleApp;
            } else if (token.equals("WindowedApp")) {
                kind = OutputKind_WindowedApp;
            } else {
                tokenizer->reportError("Invalid project kind '%s', valid values are:\n    ConsoleApp\n    WindowedApp");
                return false;
            }

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;

            if (currentConfiguration) currentConfiguration->kind = kind;
            else project->kind = kind;
        } else if (token.equals("outputdir")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_String)) return false;

            if (currentConfiguration) currentConfiguration->outputdir = toCString(token);
            else project->outputdir = toCString(token);

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("objdir")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_String)) return false;
            
            if (currentConfiguration) currentConfiguration->objdir = toCString(token);
            else project->objdir = toCString(token);

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("outputname")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_String)) return false;

            if (currentConfiguration) currentConfiguration->outputname = toCString(token);
            else project->outputname = toCString(token);

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("files")) {
            if (currentConfiguration) {
                if (!parseStringArray(tokenizer, currentConfiguration->files)) {
                    return false;
                }
            } else {
                if (!parseStringArray(tokenizer, project->files)) {
                    return false;
                }
            }
        } else if (token.equals("if")) {
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;
            char *identifierToCompare = toCString(token); // @Leak

            if (!tokenizer->expectToken(&token, "is")) return false;
            
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;
            
            char *secondIdentifierToCompare = toCString(token); // @Leak

            if (stringsMatch(identifierToCompare, "configuration")) {
                for (int i = 0; i < project->configurations.count; i++) {
                    RscConfiguration *cfg = project->configurations[i];
                    if (stringsMatch(cfg->lowercasedName, secondIdentifierToCompare)) {
                        currentConfiguration = cfg;
                        break;
                    }
                }
            }

            if (!tokenizer->expectToken(&token, TokenType_OpenBrace)) return false;
        } else if (token.equals("staticruntime")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;
            
            bool value = false;
            if (token.equals("true")) {
                value = true;
            } else if (token.equals("false")) {
                value = false;
            } else {
                tokenizer->reportError("Invalid boolean value '%s', valid values are:\n    true\n    false");
                return false;
            }
            
            if (currentConfiguration) currentConfiguration->staticRuntime = value;
            else project->staticRuntime = value;

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("staticRuntime")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;

            bool value = false;
            if (token.equals("true")) {
                value = true;
            } else if (token.equals("false")) {
                value = false;
            } else {
                tokenizer->reportError("Invalid boolean value '%s', valid values are:\n    true\n    false");
                return false;
            }
            
            if (currentConfiguration) currentConfiguration->staticRuntime = value;
            else project->staticRuntime = value;

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("optimize")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;

            bool value = false;
            if (token.equals("true")) {
                value = true;
            } else if (token.equals("false")) {
                value = false;
            } else {
                tokenizer->reportError("Invalid boolean value '%s', valid values are:\n    true\n    false");
                return false;
            }
            
            if (currentConfiguration) currentConfiguration->optimize = value;
            else project->optimize = value;

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("debugSymbols")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;

            bool value = false;
            if (token.equals("true")) {
                value = true;
            } else if (token.equals("false")) {
                value = false;
            } else {
                tokenizer->reportError("Invalid boolean value '%s', valid values are:\n    true\n    false");
                return false;
            }
            
            if (currentConfiguration) currentConfiguration->debugSymbols = value;
            else project->debugSymbols = value;

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("runtime")) {
            if (!tokenizer->expectToken(&token, TokenType_Equals)) return false;
            if (!tokenizer->expectToken(&token, TokenType_Identifier)) return false;

            RuntimeType value = RuntimeType_Debug;
            if (token.equals("Debug")) {
                value = RuntimeType_Debug;
            } else if (token.equals("Release")) {
                value = RuntimeType_Release;
            } else {
                tokenizer->reportError("Invalid runtime value '%s', valid values are:\n    Debug\n    Release");
                return false;
            }
            
            if (currentConfiguration) currentConfiguration->runtimeType = value;
            else project->runtimeType = value;

            if (!tokenizer->expectToken(&token, TokenType_Semicolon)) return false;
        } else if (token.equals("defines")) {
            if (currentConfiguration) {
                if (!parseIdentifierArray(tokenizer, currentConfiguration->defines)) {
                    return false;
                }
            } else {
                if (!parseIdentifierArray(tokenizer, project->defines)) {
                    return false;
                }
            }
        } else if (token.equals("includeDirs")) {
            if (currentConfiguration) {
                if (!parseStringArray(tokenizer, currentConfiguration->includeDirs)) {
                    return false;
                }
            } else {
                if (!parseStringArray(tokenizer, project->includeDirs)) {
                    return false;
                }
            }
        } else if (token.equals("libDirs")) {
            if (currentConfiguration) {
                if (!parseStringArray(tokenizer, currentConfiguration->libDirs)) {
                    return false;
                }
            } else {
                if (!parseStringArray(tokenizer, project->libDirs)) {
                    return false;
                }
            }
        } else if (token.equals("libs")) {
            if (currentConfiguration) {
                if (!parseStringArray(tokenizer, currentConfiguration->libs)) {
                    return false;
                }
            } else {
                if (!parseStringArray(tokenizer, project->libs)) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool parseRscFile(char *filepath, char *data) {
    Tokenizer tokenizer(filepath, data);
    
    Token token;
    if (!tokenizer.expectToken(&token, "version")) return false;
    if (!tokenizer.expectToken(&token, TokenType_Equals)) return false;
    if (!tokenizer.expectToken(&token, TokenType_Number)) return false;
    if (!tokenizer.expectToken(&token, TokenType_Semicolon)) return false;

    globalData.version = atoi(toCString(token)); // @Leak

    bool parsing = true;
    while (parsing) {
        token = tokenizer.getToken();
        if (token.type == TokenType_EOF) { parsing = false; break; }
        if (token.type != TokenType_Identifier) continue;

        if (token.equals("configurations")) {
            if (!parseStringArray(&tokenizer, globalData.configurationNames)) {
                return false;
            }
        } else if (token.equals("project")) {
            RscProject *project = new RscProject();

            for (int i = 0; i < globalData.configurationNames.count; i++) {
                RscConfiguration *cfg = new RscConfiguration();
                cfg->name = copyString(globalData.configurationNames[i]);
                cfg->lowercasedName = copyStringLowercased(globalData.configurationNames[i]);
                project->configurations.add(cfg);
            }
            
            if (!parseProject(&tokenizer, project)) {
                return false;
            }
            globalData.projects.add(project);
        }
    }

    return true;
}
