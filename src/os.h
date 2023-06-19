#pragma once

#include "defines.h"

namespace os {

    void *readEntireFile(char *filepath, i64 *lengthPointer = NULL);

    bool fileExists(char *filepath);
    bool getLastWriteTime(char *filepath, u64 *outTime);
    bool deleteFile(char *file);
        
    bool directoryExists(char *filepath);
    bool makeDirectoryIfNotExist(char *dir);

    double getTime();

    bool copyFile(char *sourceFile, char *destFile);
    
}
