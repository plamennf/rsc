#pragma once

#include "dynamic_array.h"

enum OutputKind {
    OutputKind_ConsoleApp,
    OutputKind_WindowedApp,
    OutputKind_StaticLib,
};

enum RuntimeType {
    RuntimeType_Debug,
    RuntimeType_Release,
};

struct RscConfiguration {
    char *name = NULL;
    char *lowercasedName = NULL;
    
    OutputKind kind = OutputKind_ConsoleApp;
    char *outputdir = NULL;
    char *objdir = NULL;
    char *outputname = NULL;
    
    DynamicArray<char *> files;
    DynamicArray<char *> defines;

    DynamicArray<char *> includeDirs;
    DynamicArray<char *> libDirs;
    DynamicArray<char *> libs;

    char *pchheader = NULL;
    char *pchsource = NULL;

    bool debugSymbols = true;
    bool debugSymbolsSet = false;
    bool optimize = false;
    bool optimizeSet = false;
    bool staticRuntime = true;
    bool staticRuntimeSet = false;
    RuntimeType runtimeType = RuntimeType_Debug;
};

struct RscProject : public RscConfiguration {
    DynamicArray<RscConfiguration *> configurations;
};

struct GlobalData {
    char *filename = NULL;
    char *configurationNameToBuild = NULL;
    bool rebuild = false;
    
    int version = -1;
    
    DynamicArray<char *> configurationNames;
    DynamicArray<RscProject *> projects;
};

extern GlobalData globalData;
