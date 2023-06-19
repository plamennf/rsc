#include "os.h"
#include "dynamic_array.h"
#include "utils.h"

#include <windows.h>

static void toWindowsFilepath(char *filepath, wchar_t *wideFilepath, i32 wideFilepathSize) {
    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wideFilepath, wideFilepathSize);

    for (wchar_t *at = wideFilepath; at[0]; at++) {
        if (at[0] == '/') {
            at[0] = '\\';
        }
    }
}

void *os::readEntireFile(char *filepath, i64 *lengthPointer) {
    wchar_t wideFilepath[4096];
    toWindowsFilepath(filepath, wideFilepath, ArrayCount(wideFilepath));
    
    HANDLE fileHandle = CreateFileW(wideFilepath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!fileHandle) {
        if (lengthPointer) *lengthPointer = 0;
        return NULL;
    }
    defer { CloseHandle(fileHandle); };

    i64 length;
    GetFileSizeEx(fileHandle, (LARGE_INTEGER *)&length);
    if (lengthPointer) *lengthPointer = length;
    
    void *data = malloc(length + 1);
    memset(data, 0, length + 1);

    DWORD bytesRead = 0;
    ReadFile(fileHandle, data, (DWORD)length, &bytesRead, NULL);
    
    return data;
}

bool os::fileExists(char *filepath) {
    wchar_t wideFilepath[4096];
    toWindowsFilepath(filepath, wideFilepath, ArrayCount(wideFilepath));

    DWORD attrib = GetFileAttributesW(wideFilepath);
    return (attrib != INVALID_FILE_ATTRIBUTES &&
            !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool os::directoryExists(char *filepath) {
    wchar_t wideFilepath[4096];
    toWindowsFilepath(filepath, wideFilepath, ArrayCount(wideFilepath));

    DWORD attrib = GetFileAttributesW(wideFilepath);
    return (attrib != INVALID_FILE_ATTRIBUTES &&
            (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static void splitDirectoryIntoSourceDirectories(char *dir, DynamicArray<char *> &dirs) {
    char *at = dir;
    while (true) {
        char *end = strchr(at, '\\');
        if (!end) {
            dirs.add(copyString(dir));
            break;
        }
        
        char *s = copyString(dir);
        s[end - dir] = 0;
        
        dirs.add(s);

        end++;
        at = end;
    }
}

bool os::makeDirectoryIfNotExist(char *dir) {
    if (os::directoryExists(dir)) return false;

    for (char *at = dir; *at; *at++) {
        if (*at == '/') {
            *at = '\\';
        }
    }

    DynamicArray<char *> dirs;
    splitDirectoryIntoSourceDirectories(dir, dirs);

    for (int i = 0; i < dirs.count; i++) {
        char *d = dirs[i];

        wchar_t wideFilepath[4096];
        toWindowsFilepath(d, wideFilepath, ArrayCount(wideFilepath));
    
        for (wchar_t *at = wideFilepath; *at; at++) {
            if (*at == L'/') {
                *at = L'\\';
            }
        }
        
        CreateDirectoryW(wideFilepath, NULL);
    }

    return true;
}

bool os::getLastWriteTime(char *filepath, u64 *outTime) {
    wchar_t wideFilepath[4096];
    toWindowsFilepath(filepath, wideFilepath, ArrayCount(wideFilepath));
    
    HANDLE file = CreateFileW(wideFilepath, GENERIC_READ,
                              FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL,
                              OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    defer { CloseHandle(file); };

    FILETIME ftCreate, ftAccess, ftWrite;

    if (!GetFileTime(file, &ftCreate, &ftAccess, &ftWrite))
        return false;

    ULARGE_INTEGER uli;
    uli.LowPart = ftWrite.dwLowDateTime;
    uli.HighPart = ftWrite.dwHighDateTime;
    
    if (outTime) *outTime = uli.QuadPart;

    return true;
}

bool os::deleteFile(char *filepath) {
    wchar_t wideFilepath[4096];
    toWindowsFilepath(filepath, wideFilepath, ArrayCount(wideFilepath));
    
    return DeleteFileW(wideFilepath);
}

double os::getTime() {
    i64 perfCounter;
    QueryPerformanceCounter((LARGE_INTEGER *)&perfCounter);

    i64 perfFreq;
    QueryPerformanceFrequency((LARGE_INTEGER *)&perfFreq);

    return (double)perfCounter / (double)perfFreq;
}

bool os::copyFile(char *sourceFile, char *destFile) {
    wchar_t wideSourceFilepath[4096];
    toWindowsFilepath(sourceFile, wideSourceFilepath, ArrayCount(wideSourceFilepath));

    wchar_t wideDestFilepath[4096];
    toWindowsFilepath(destFile, wideDestFilepath, ArrayCount(wideDestFilepath));

    BOOL result = CopyFileW(wideSourceFilepath, wideDestFilepath, FALSE);
    return result;
}
