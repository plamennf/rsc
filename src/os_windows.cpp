#include <windows.h>

char *read_entire_file(char *file_path) {
    HANDLE file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;

    LARGE_INTEGER length;
    GetFileSizeEx(file, &length);

    char *result = new char[length.QuadPart + 1];

    DWORD bytes_read = 0;
    ReadFile(file, result, (DWORD)length.QuadPart, &bytes_read, NULL);

    result[length.QuadPart] = 0;
    
    return result;
}

u64 get_last_write_time(char *file_path) {
    HANDLE file = CreateFileA(file_path, 0, FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;

    FILETIME ft_create, ft_access, ft_write;
    GetFileTime(file, &ft_create, &ft_access, &ft_write);

    LARGE_INTEGER li;
    li.LowPart = ft_write.dwLowDateTime;
    li.HighPart = ft_write.dwHighDateTime;

    return li.QuadPart;
}

char *get_current_directory() {
    char directory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, directory);
    return copy_string(directory);
}

bool file_exists(char *path) {
    DWORD result = GetFileAttributes(path);
    return result != INVALID_FILE_ATTRIBUTES;
}

u64 get_performance_counter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

u64 get_performance_frequency() {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

double get_time() {
    return (double)get_performance_counter() / (double)get_performance_frequency();
}
