#include "main.h"
#include "utils.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double rscStartTime = 0.0;

static char *replaceForwardslashWithBackslash(char *s) {
    if (!s) return NULL;

    for (char *at = s; *at; at++) {
        if (at[0] == '/') {
            at[0] = '\\';
        }
    }

    return s;
}

static char *replaceBackslashWithForwardslash(char *s) {
    if (!s) return NULL;

    for (char *at = s; *at; at++) {
        if (at[0] == '\\') {
            at[0] = '/';
        }
    }

    return s;
}

static char *doMacroSubstitutions(char *s, RscProject *project, RscConfiguration *configuration) {
    StringBuilder sanitized;
    for (char *at = s; *at;) {
        if (at[0] == '%') {
            if (at[1] != '{') {
                fprintf(stderr, "Macros always start with %%{ and end with }\n");
                exit(1);
            }
            at += 2;

            StringBuilder macroNameBuilder;
            while (1) {
                if (at[0] == 0) {
                    fprintf(stderr, "EOF found while parsing string.\n");
                    exit(1);
                }
                if (at[0] == '}') break;

                macroNameBuilder.add(at[0]);
                at++;
            }
            at++;

            char *macroName = macroNameBuilder.toString(); // @Leak
            defer { delete[] macroName; };

            if (stringsMatch(macroName, "ProjectName")) {
                sanitized.add(project->name);
            } else if (stringsMatch(macroName, "Configuration")) {
                sanitized.add(configuration->name);
            } else if (stringsMatch(macroName, "OutputDir")) {
                char *outputdir = mprintf("build\\%s", configuration->name);
                if (project->outputdir) {
                    delete[] outputdir;
                    outputdir = project->outputdir;
                }
                if (configuration->outputdir) {
                    delete[] outputdir;
                    outputdir = configuration->outputdir;
                }
                replaceForwardslashWithBackslash(outputdir);
                outputdir = doMacroSubstitutions(outputdir, project, configuration);
                sanitized.add(outputdir);
            } else {
                fprintf(stderr, "Invalid value for macro. Valid values are:\n");
                fprintf(stderr, "   ProjectName\n");
                fprintf(stderr, "   Configuration\n");
                fprintf(stderr, "   OutputDir\n");
                exit(1);
            }
        } else {
            sanitized.add(at[0]);
            at++;
        }
    }

    return sanitized.toString();
}

static char *getDirectoryFromFilename(char *string) {
    if (!string) return NULL;

    char *result = copyString(string);
    
    char *slash = strrchr(result, '/');
    result[slash - result] = 0;
    return result;
}

static void checkFileForIncludes(char *filename, DynamicArray<char *> &includes) {
    char *data = (char *)os::readEntireFile(filename);
    if (!data) return;
    defer { delete [] data; };

    char *at = data;
    while (1) {
        char *line = consumeNextLine(&at);
        if (!line) break;

        line = eatWhitespace(line);

        if (!startsWith(line, "#include")) continue;
            
        line += getStringLength("#include");
        line = eatWhitespace(line);

        if (line[0] != '"' && line[0] != '<') {
            //fprintf(stderr, "Shit happened\n");
            continue;
        }

        line++;
        
        DynamicArray<char> includeNameString;
        while (1) {
            if (line[0] == 0) {
                fprintf(stderr, "EOF found while parsing string in c file.");
                continue;
            }
            if (line[0] == '>' || line[0] == '"') break;

            includeNameString.add(line[0]);
            line++;
        }
        includeNameString.add(0);

        char *fileNameDirectory = getDirectoryFromFilename(filename);
        
        DynamicArray<char *> directories;
        directories.add(fileNameDirectory);
        
        bool fileExists = false;
        char *fullPath = NULL;
        for (int i = 0; i < directories.count; i++) {
            char *dir = directories[i];
            if (!dir) {
                fileExists = os::fileExists(filename);
                continue;
            }

            char *path = mprintf("%s/%s", dir, includeNameString.data);
            fileExists = os::fileExists(path);
            if (fileExists) {
                fullPath = path;
            }
        }

        if (fileExists) {
            includes.add(fullPath);
            checkFileForIncludes(fullPath, includes);
        }
    }
}

void executeMSVCForProject(RscProject *project, RscConfiguration *configuration, u64 rscModtime) {
    char *outputdir = mprintf("build\\%s", configuration->name);
    if (project->outputdir) outputdir = project->outputdir;
    if (configuration->outputdir) outputdir = configuration->outputdir;
    replaceForwardslashWithBackslash(outputdir);
    outputdir = doMacroSubstitutions(outputdir, project, configuration); // @Leak
    
    char *objdir = mprintf("obj\\%s\\%s", project->name, configuration->name);
    if (project->objdir) objdir = project->objdir;
    if (configuration->objdir) objdir = configuration->objdir;
    replaceForwardslashWithBackslash(objdir);
    objdir = doMacroSubstitutions(objdir, project, configuration); // @Leak

    char *outputname = project->name;
    if (project->outputname) outputname = project->outputname;
    if (configuration->outputname) outputname = configuration->outputname;
    replaceForwardslashWithBackslash(outputname);
    outputname = doMacroSubstitutions(outputname, project, configuration); // @Leak
    
    u64 exeModtime = 0;
    char *exepath = mprintf("%s/%s.exe", outputdir, outputname);
    os::getLastWriteTime(exepath, &exeModtime);

    char *pchheader = project->pchheader;
    if (configuration->pchheader) pchheader = configuration->pchheader;
    pchheader = replaceForwardslashWithBackslash(pchheader);

    char *pchsource = project->pchsource;
    if (configuration->pchsource) pchsource = configuration->pchsource;
    pchsource = replaceForwardslashWithBackslash(pchsource);
    
    if (rscModtime > exeModtime) {
        globalData.rebuild = true;
    }

    os::makeDirectoryIfNotExist(outputdir);
    os::makeDirectoryIfNotExist(objdir);
    
    DynamicArray<char *> filesToCompile;
    for (int i = 0; i < project->files.count; i++) {
        char *filename = project->files[i];

        if (globalData.rebuild) {
            filesToCompile.add(filename);
            continue;
        }
        
        DynamicArray<char *> fileIncludes;
        checkFileForIncludes(filename, fileIncludes);

        u64 latestModtime = 0;
        os::getLastWriteTime(filename, &latestModtime);

        for (int i = 0; i < fileIncludes.count; i++) {
            char *includename = fileIncludes[i];
            u64 includeModtime = 0;
            os::getLastWriteTime(includename, &includeModtime);

            if (includeModtime > latestModtime) {
                latestModtime = includeModtime;
            }
        }

        if (latestModtime > exeModtime) {
            filesToCompile.add(filename);
        }
    }

    if (!filesToCompile.count) return;
    
    StringBuilder compilerLine;
    compilerLine.add("cl /c /nologo /W3 /diagnostics:column /WL /FC /Oi /EHsc /Zc:strictStrings- /std:c++20 /Zc:strictStrings- /D_CRT_SECURE_NO_WARNINGS ");

    bool debugSymbols = project->debugSymbols;
    if (configuration->debugSymbolsSet) debugSymbols = configuration->debugSymbols;
    bool optimize = project->optimize;
    if (configuration->optimizeSet) optimize = configuration->optimize;
    bool staticRuntime = project->staticRuntime;
    if (configuration->staticRuntimeSet) staticRuntime = configuration->staticRuntime;

    RuntimeType runtimeType = project->runtimeType;
    if (staticRuntime) {
        compilerLine.add("/MT");
    } else {
        compilerLine.add("/MD");
    }

    if (runtimeType == RuntimeType_Debug) {
        compilerLine.add('d');
    }

    compilerLine.add(' ');

    if (optimize) {
        compilerLine.add("/O2 /Ob2 ");
    } else {
        compilerLine.add("/Od /Ob0 ");
    }

    if (debugSymbols) {
        compilerLine.add("/Zi /DEBUG ");
    }

    if (pchheader && !pchsource) {
        fprintf(stderr, "pchheader set, but pchsource isn't\n");
        exit(1);
    }

    if (pchsource && !pchheader) {
        fprintf(stderr, "pchsource set, but pchheader isn't\n");
        exit(1);
    }

    DynamicArray<char *> includeDirs;
    for (int i = 0; i < project->includeDirs.count; i++) {
        char *dir = project->includeDirs[i];
        includeDirs.add(dir);
    }
    for (int i = 0; i < configuration->includeDirs.count; i++) {
        char *dir = configuration->includeDirs[i];
        includeDirs.add(dir);
    }
    
    DynamicArray<char *> libDirs;
    for (int i = 0; i < project->libDirs.count; i++) {
        char *dir = project->libDirs[i];
        libDirs.add(dir);
    }
    for (int i = 0; i < configuration->libDirs.count; i++) {
        char *dir = configuration->libDirs[i];
        libDirs.add(dir);
    }

    DynamicArray<char *> libs;
    for (int i = 0; i < project->libs.count; i++) {
        char *dir = project->libs[i];
        libs.add(dir);
    }
    for (int i = 0; i < configuration->libs.count; i++) {
        char *dir = configuration->libs[i];
        libs.add(dir);
    }

    DynamicArray<char *> defines;
    for (int i = 0; i < project->defines.count; i++) {
        char *dir = project->defines[i];
        defines.add(dir);
    }
    for (int i = 0; i < configuration->defines.count; i++) {
        char *dir = configuration->defines[i];
        defines.add(dir);
    }
    
    for (int i = 0; i < includeDirs.count; i++) {
        char *dir = includeDirs[i];
        replaceForwardslashWithBackslash(dir);
        
        dir = doMacroSubstitutions(dir, project, configuration); // @Leak
        compilerLine.printf("/I %s ", dir);
    }
    
    compilerLine.printf("/Fo\"%s\\\\\" ", objdir);
    compilerLine.printf("/Fd\"%s\\\\\" ", outputdir); // Make the .pdb file go into the output directory
    
    for (int i = 0; i < defines.count; i++) {
        char *define = defines[i];
        compilerLine.printf("/D%s ", define);
    }

    if (pchsource && pchheader) {
        compilerLine.printf("/Fp%s\\%s.pch ", outputdir, project->name);
        
        StringBuilder pchLine;
        pchLine.copyFrom(&compilerLine);
        
        pchLine.printf("/Yc\"%s\" %s ", pchheader, pchsource);

        system(pchLine.toString()); // @Leak
  
        compilerLine.printf("/Yu\"%s\" ", pchheader);
    }
    
    for (int i = 0; i < filesToCompile.count; i++) {
        char *filename = filesToCompile[i];
        compilerLine.add(filename);
        compilerLine.add(' ');
    }
    
    StringBuilder linkerLine;
    {
        if (project->kind == OutputKind_StaticLib) {
            linkerLine.add("lib ");
        } else {
            linkerLine.add("link ");
        }
        
        linkerLine.add("/nologo /MACHINE:X64 ");
    }
    
    if (pchsource) {
        project->files.add(pchsource);
    }
    
    for (int i = 0; i < project->files.count; i++) {
        char *filename = project->files[i];

        char *dirWithName = copyStripExtension(filename);
        dirWithName = replaceBackslashWithForwardslash(dirWithName);
        char *slash = strrchr(dirWithName, '/');
        
        StringBuilder name;
        if (slash) {
            for (char *at = slash + 1; *at; at++) {
                name.add(*at);
            }
        } else {
            name.add(dirWithName);
        }
        
        linkerLine.printf("%s\\%s.obj ", objdir, name.toString()); // @Leak
    }
    
    for (int i = 0; i < libs.count; i++) {
        char *lib = libs[i];
        replaceForwardslashWithBackslash(lib);
        linkerLine.printf("%s ", lib);
    }

    for (int i = 0; i < libDirs.count; i++) {
        char *dir = libDirs[i];
        replaceForwardslashWithBackslash(dir);

        dir = doMacroSubstitutions(dir, project, configuration); // @Leak
        
        linkerLine.printf("/LIBPATH:\"%s\" ", dir);
    }
    
    if (debugSymbols && project->kind != OutputKind_StaticLib) {
        linkerLine.add("/DEBUG ");
    }
    
    if (project->kind == OutputKind_ConsoleApp) {
        linkerLine.add("/subsystem:console ");
    } else if (project->kind == OutputKind_WindowedApp) {
        linkerLine.add("/subsystem:windows ");
    }

    {
        char *extension = "exe";
        if (project->kind == OutputKind_StaticLib) {
            extension = "lib";
        }
        linkerLine.printf("/OUT:%s\\%s.%s ", outputdir, outputname, extension);
    }

    double rscEndTime = os::getTime();
    double rscTime = rscEndTime - rscStartTime;

    double clStartTime = os::getTime();
    printf("Compiler line: %s\n", compilerLine.toString()); // @Leak
    int result = system(compilerLine.toString()); // @Leak
#ifndef _DEBUG
    if (result != 0) {
        os::deleteFile(exepath);
        exit(1);
    }
#endif
    double clEndTime = os::getTime();
    double clTime = clEndTime - clStartTime;

    double linkerStartTime = os::getTime();
    printf("Linker line: %s\n", linkerLine.toString()); // @Leak
    result = system(linkerLine.toString()); // @Leak
#ifndef _DEBUG
    if (result != 0) {
        os::deleteFile(exepath);
        exit(1);
    }
#endif
    double linkerEndTime = os::getTime();
    double linkerTime = linkerEndTime - linkerStartTime;

    printf("Total time: %.4f\n", rscTime + clTime + linkerTime);
    printf("RSC time: %.4f\n", rscTime);
    printf("MSVC time: %.4f\n", clTime);
    printf("Linker time: %.4f\n", linkerTime);
}
