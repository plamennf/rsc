#ifdef _WIN32

#include "os.h"
#include "general.h"
#include "ds.h"

#include <windows.h>
#include <string.h> // For memset

wchar_t *utf8_to_wstring(char *string) {
    int required_size = MultiByteToWideChar(CP_UTF8, 0, string, -1, nullptr, 0);
    if (!required_size) return nullptr;

    wchar_t *result = static_cast <wchar_t *>(talloc((required_size + 1) * sizeof(wchar_t)));

    MultiByteToWideChar(CP_UTF8, 0, string, -1, result, required_size);
    
    return result;
}

char *wstring_to_utf8(wchar_t *string) {
    int required_size = WideCharToMultiByte(CP_UTF8, 0, string, -1, nullptr, 0, nullptr, nullptr);
    if (!required_size) return nullptr;
    
    char *result = static_cast <char *>(talloc(required_size + 1));
    WideCharToMultiByte(CP_UTF8, 0, string, -1, result, required_size, nullptr, nullptr);
    
    return result;
}

char *os_read_entire_file(char *filepath, s64 *out_length) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    wchar_t *wide_filepath = utf8_to_wstring(filepath);

    HANDLE file = CreateFileW(wide_filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        if (out_length) *out_length = 0;
        return nullptr;
    }
    defer { CloseHandle(file); };
    
    s64 length = 0;
    GetFileSizeEx(file, (LARGE_INTEGER *)&length);

    if (out_length) *out_length = length;

    char *result = new char[length + 1];
    memset(result, 0, (length + 1) * sizeof(char));

    DWORD num_bytes_read = 0;
    ReadFile(file, result, (DWORD)length, &num_bytes_read, nullptr);

    return result;
}

bool os_file_exists(char *filepath) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    wchar_t *wide_filepath = utf8_to_wstring(filepath);
    for (wchar_t *at = wide_filepath; *at; at++) {
        if (*at == L'/') {
            *at = L'\\';
        }
    }
    
    DWORD attrib = GetFileAttributesW(wide_filepath);

    return (attrib != INVALID_FILE_ATTRIBUTES &&
            !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

double os_get_time() {
    s64 perf_count;
    QueryPerformanceCounter((LARGE_INTEGER *)&perf_count);

    s64 perf_freq;
    QueryPerformanceFrequency((LARGE_INTEGER *)&perf_freq);

    return (double)perf_count / (double)perf_freq;
}

void os_setcwd(char *_dir) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    char *dir = copy_string(_dir, true);
    
    for (char *at = dir; *at; at++) {
        if (at[0] == '/') {
            at[0] = '\\';
        }
    }

    wchar_t *wide_dir = utf8_to_wstring(dir);
    
    SetCurrentDirectoryW(wide_dir);
}

void os_init_colors_and_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode = 0;
    BOOL result = GetConsoleMode(stdout, &mode);
    if (result) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(stdout, mode);
    }
}

bool os_directory_exists(char *dir) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    wchar_t *wide_filepath = utf8_to_wstring(dir);
    for (wchar_t *at = wide_filepath; *at; at++) {
        if (*at == L'/') {
            *at = L'\\';
        }
    }

    DWORD attrib = GetFileAttributesW(wide_filepath);

    return (attrib != INVALID_FILE_ATTRIBUTES && 
            (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static void split_directory_into_source_directories(char *dir, Array <char *> &dirs, bool use_temporary_storage = true) {
    char *at = dir;
    while (true) {
        char *end = strchr(at, '\\');
        if (!end) {
            dirs.add(copy_string(at, use_temporary_storage));
            break;
        }
        
        char *s = copy_string(dir, use_temporary_storage);
        s[end - dir] = 0;

        dirs.add(s);

        end++;
        at = end;
    }
}

void os_make_directory_if_not_exist(char *dir) {
    if (os_directory_exists(dir)) return;

    for (char *at = dir; *at; *at++) {
        if (*at == '/') {
            *at = '\\';
        }
    }
    
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };

    Array <char *> dirs;
    dirs.use_temporary_storage = true;
    split_directory_into_source_directories(dir, dirs);

    for (char *d : dirs) {
        wchar_t *wide_filepath = utf8_to_wstring(d);
        for (wchar_t *at = wide_filepath; *at; at++) {
            if (*at == L'/') {
                *at = L'\\';
            }
        }
        
        CreateDirectoryW(wide_filepath, NULL);
    }
}

void os_get_last_write_time(char *file_path, u64 *out_time) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    wchar_t *wide_filepath = utf8_to_wstring(file_path);
    HANDLE file = CreateFileW(wide_filepath, GENERIC_READ,
                              FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    defer { CloseHandle(file); };

    FILETIME ft_create, ft_access, ft_write;

    if (!GetFileTime(file, &ft_create, &ft_access, &ft_write))
        return;

    ULARGE_INTEGER uli;
    uli.LowPart = ft_write.dwLowDateTime;
    uli.HighPart = ft_write.dwHighDateTime;
    
    *out_time = uli.QuadPart;
}

bool os_delete_file(char *file) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    wchar_t *wide_filepath = utf8_to_wstring(file);
    
    return DeleteFileW(wide_filepath);
}

#endif
