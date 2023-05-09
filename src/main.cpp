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

    if (!startsWith(argv[2], "-configuration:")) {
        fprintf(stderr, "Second parameter must start with -configuration:.\n");
        printUsage();
        return false;
    }

    char *stringToCopy = argv[2];
    stringToCopy += getStringLength("-configuration:");
    globalData.configurationNameToBuild = copyString(stringToCopy);
    
    if (argc == 4) {
        if (stringsMatch(argv[3], "-B")) {
            globalData.rebuild = true;
        }
    }

    return true;
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

    u64 rscModtime = 0;
    os::getLastWriteTime(globalData.filename, &rscModtime);

    char *currentConfigurationName = NULL;
    char *cfgName = copyStringLowercased(globalData.configurationNameToBuild);
    for (int i = 0; i < globalData.configurationNames.count; i++) {
        char *cfg = copyStringLowercased(globalData.configurationNames[i]);
        defer { delete[] cfg; };
        
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
