#include "utils.h"
#include "main.h"
#include "os.h"

#include <stdio.h>

GlobalData globalData = {};

bool parseRscFile(char *filepath, char *data);
void executeMSVCForProject(RscProject *project, RscConfiguration *cfg, u64 rscModtime);

static bool isValid(GlobalData data) {
    return ((data.filename != NULL) &&
            (data.configurationNameToBuild != NULL));
}

static void printUsage() {
    printf("Usage: rsc <filename>.rsc -configuration:<ConfigurationName> [-B]\n");
}

static bool parseCommandLineArguments(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "No input file provided.\n");
        printUsage();
        return false;
    }

    if (argc < 3) {
        fprintf(stderr, "No configuration to build provided.\n");
        printUsage();
        return false;
    }

    globalData.filename = copyString(argv[1]);
    globalData.configurationNameToBuild = copyString(argv[2] + getStringLength("-configuration:"));

    if (argc == 4) {
        if (stringsMatch(argv[3], "-B")) {
            globalData.rebuild = true;
        }
    }

    return true;
}

static void printGlobalData() {
    printf("configurations = {\n");
    for (int i = 0; i < globalData.configurationNames.count; i++) {
        printf("    \"%s\",\n", globalData.configurationNames[i]);
    }
    printf("};\n\n");

    for (int i = 0; i < globalData.projects.count; i++) {
        RscProject *project = globalData.projects[i];
        printf("project '%s' {\n", project->name);

        printf("    kind = '%d';\n", project->kind);
        printf("    outputdir = '%s';\n", project->outputdir);
        printf("    objdir = '%s';\n", project->objdir);
        printf("    outputname = '%s';\n", project->outputname);

        printf("    staticRuntime = '%d';\n", project->staticRuntime);

        printf("    debugSymbols = '%d';\n", project->debugSymbols);
        printf("    optimize = '%d';\n", project->optimize);
        printf("    runtime = '%d';\n", project->runtimeType);
        
        printf("    files = {\n");
        for (int j = 0; j < project->files.count; j++) {
            printf("        \"%s\",\n", project->files[j]);
        }
        for (int j = 0; j < project->defines.count; j++) {
            printf("        \"%s\",\n", project->defines[j]);
        }
        for (int j = 0; j < project->includeDirs.count; j++) {
            printf("        \"%s\",\n", project->includeDirs[j]);
        }
        for (int j = 0; j < project->libDirs.count; j++) {
            printf("        \"%s\",\n", project->libDirs[j]);
        }
        for (int j = 0; j < project->libs.count; j++) {
            printf("        \"%s\",\n", project->libs[j]);
        }
        printf("    };\n");
        
        printf("}\n");

        for (int j = 0; j < project->configurations.count; j++) {
            RscConfiguration *cfg = project->configurations[j];
            printf("configuration '%s' {\n", cfg->name);
            printf("    kind = '%d';\n", cfg->kind);
            printf("    outputdir = '%s';\n", cfg->outputdir);
            printf("    objdir = '%s';\n", cfg->objdir);
            printf("    outputname = '%s';\n", cfg->outputname);

            printf("    debugSymbols = '%d';\n", cfg->debugSymbols);
            printf("    optimize = '%d';\n", cfg->optimize);
            printf("    runtime = '%d';\n", cfg->runtimeType);
            
            printf("    files = {\n");
            for (int j = 0; j < cfg->files.count; j++) {
                printf("        \"%s\",\n", cfg->files[j]);
            }
            for (int j = 0; j < cfg->defines.count; j++) {
                printf("        \"%s\",\n", cfg->defines[j]);
            }
            for (int j = 0; j < cfg->includeDirs.count; j++) {
                printf("        \"%s\",\n", cfg->includeDirs[j]);
            }
            for (int j = 0; j < cfg->libDirs.count; j++) {
                printf("        \"%s\",\n", cfg->libDirs[j]);
            }
            for (int j = 0; j < cfg->libs.count; j++) {
                printf("        \"%s\",\n", cfg->libs[j]);
            }
            printf("    };\n");
            
            printf("}\n");
        }
    }
}

int main(int argc, char **argv) {
    if (!parseCommandLineArguments(argc, argv)) return 1;
    Assert(isValid(globalData));

    extern double rscStartTime;
    rscStartTime = os::getTime();
    
    char *fileData = (char *)os::readEntireFile(globalData.filename);
    if (!fileData) return 1;
    
    if (!parseRscFile(globalData.filename, fileData)) {
        return 1;
    }

    // printGlobalData();

    u64 rscModtime = 0;
    os::getLastWriteTime(globalData.filename, &rscModtime);

    char *currentConfigurationName = NULL;
    char *cfgName = copyStringLowercased(globalData.configurationNameToBuild);
    for (int i = 0; i < globalData.configurationNames.count; i++) {
        char *cfg = copyStringLowercased(globalData.configurationNames[i]);

        if (stringsMatch(cfg, cfgName)) {
            currentConfigurationName = cfgName;
            break;
        }
    }
    
    if (!currentConfigurationName) {
        fprintf(stderr, "ERROR: configuration passed to -configuration: isn't a valid configuration\n");
        return 1;
    }
    
    for (int i = 0; i < globalData.projects.count; i++) {
        RscProject *project = globalData.projects[i];

        RscConfiguration *currentConfiguration = NULL;
        for (int j = 0; j < project->configurations.count; j++) {
            RscConfiguration *cfg = project->configurations[j];
            if (stringsMatch(cfg->lowercasedName, currentConfigurationName)) {
                currentConfiguration = cfg;
                break;
            }
        }

        executeMSVCForProject(project, currentConfiguration, rscModtime);
    }
    
    return 0;
}
