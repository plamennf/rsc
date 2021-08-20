#include "core.h"
#include <windows.h>

static Array<char> compiler_line(1024);

struct Rsc_Dir {
    char name[1024] = {};
};

struct Configuration {
    char name[1024] = {};

    char outputdir[1024] = {};
    bool outputdir_set = false;
    char objdir[1024] = {};
    bool objdir_set = false;
    char exename[1024] = {};
    bool exename_set = false;

    Array<Rsc_Dir> includedirs;
    bool includedirs_set = false;
    Array<Rsc_Dir> libdirs;
    bool libdirs_set = false;
    Array<Rsc_Dir> libs;
    bool libs_set = false;

    char subsystem[1024] = {};
    bool subsystem_set = false;
};

struct Rsc_Data {
    char outputdir[1024] = {};
    bool outputdir_set = false;
    char objdir[1024] = {};
    bool objdir_set = false;
    char exename[1024] = {};
    bool exename_set = false;

    Array<Rsc_Dir> includedirs;
    bool includedirs_set = false;
    Array<Rsc_Dir> libdirs;
    bool libdirs_set = false;
    Array<Rsc_Dir> libs;
    bool libs_set = false;

    Array<Configuration> configurations;
    Array<Rsc_Dir> files;

    char subsystem[1024] = {};
    bool subsystem_set = false;
};

static void parse_rsc_file(char *file_path, Rsc_Data *out_data) {
    char *data = read_entire_file(file_path);
    defer(delete[] data);
    
    char *at = data;
    bool is_in_configuration = false;
    Configuration *current_configuration = NULL;
    bool is_in_includedirs = false;
    bool is_in_libdirs = false;
    bool is_in_libs = false;
    bool is_in_files = false;
    while (1) {
        char *line = consume_next_line(&at);
        if (!line) break;

        if (strstr(line, "outputdir")) {
            char *cursor = strstr(line, "outputdir");
            cursor += strlen("outputdir ");

            if (is_in_configuration && current_configuration) {
                int i = 0;
                while (*cursor != '\n') {
                    current_configuration->outputdir[i] = *cursor;
                    cursor++;
                    i++;
                }
                current_configuration->outputdir_set = true;                
            } else {
                int i = 0;
                while (*cursor != '\n') {
                    out_data->outputdir[i] = *cursor;
                    cursor++;
                    i++;
                }
                out_data->outputdir_set = true;
            }
        } else if (strstr(line, "objdir")) {
            char *cursor = strstr(line, "objdir");
            cursor += strlen("objdir ");

            if (is_in_configuration && current_configuration) {
                int i = 0;
                while (*cursor != '\n') {
                    current_configuration->objdir[i] = *cursor;
                    cursor++;
                    i++;
                }
                current_configuration->objdir_set = true;
            } else {
                int i = 0;
                while (*cursor != '\n') {
                    out_data->objdir[i] = *cursor;
                    cursor++;
                    i++;
                }
                out_data->objdir_set = true;
            }
        } else if (strstr(line, "exename")) {
            char *cursor = strstr(line, "exename");
            cursor += strlen("exename ");

            if (is_in_configuration && current_configuration) {
                int i = 0;
                while (*cursor != '\n') {
                    current_configuration->exename[i] = *cursor;
                    cursor++;
                    i++;
                }
                current_configuration->exename_set = true;
            } else {
                int i = 0;
                while (*cursor != '\n') {
                    out_data->exename[i] = *cursor;
                    cursor++;
                    i++;
                }
                out_data->exename_set = true;
            }
        } else if (strstr(line, "subsystem")) {
            char *cursor = strstr(line, "subsystem");
            cursor += strlen("subsystem ");

            if (is_in_configuration && current_configuration) {
                int i = 0;
                while (*cursor != '\n') {
                    current_configuration->subsystem[i] = *cursor;
                    cursor++;
                    i++;
                }
                current_configuration->subsystem_set = true;
            } else {
                int i = 0;
                while (*cursor != '\n') {
                    out_data->subsystem[i] = *cursor;
                    cursor++;
                    i++;
                }
                out_data->subsystem_set =true;
            }
        } else if (strstr(line, "configurations")) {
            is_in_configuration = true;
        } else if (strstr(line, "}")) {
            is_in_configuration = false;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = false;
            is_in_files = false;
        } else if (strstr(line, "includedirs")) {
            is_in_includedirs = true;
            if (is_in_configuration) current_configuration->includedirs_set = true;
            else out_data->includedirs_set = true;
        } else if (strstr(line, "libdirs")) {
            is_in_libdirs = true;
            if (is_in_configuration) current_configuration->libdirs_set = true;
            else out_data->libdirs_set = true;
        } else if (strstr(line, "libs")) {
            is_in_libs = true;
            if (is_in_configuration) current_configuration->libs_set = true;
            else out_data->libs_set = true;
        } else if (strstr(line, "files")) {
            is_in_files = true;
        } else {
            if (is_in_configuration) {
                out_data->configurations.add({});
                current_configuration = &out_data->configurations[out_data->configurations.count - 1];
                int i = 0;
                while (*line != ':') {
                    if (*line == ' ') { line++; continue; }
                    current_configuration->name[i] = *line;
                    line++;
                    i++;
                }
            } else if (is_in_includedirs) {
                char dir_name[1024] = {};
                int i = 0;
                while (*line != '\n') {
                    if (*line == ' ') { line++; continue; }
                    dir_name[i] = *line;
                    line++;
                    i++;
                }
                Rsc_Dir dir = {};
                strcpy(dir.name, dir_name);
                if (is_in_configuration && current_configuration) {
                    current_configuration->includedirs.add(dir);
                } else {
                    out_data->includedirs.add(dir);
                }
            } else if (is_in_libdirs) {
                char dir_name[1024] = {};
                int i = 0;
                while (*line != '\n') {
                    if (*line == ' ') { line++; continue; }
                    dir_name[i] = *line;
                    line++;
                    i++;
                }
                Rsc_Dir dir = {};
                strcpy(dir.name, dir_name);
                if (is_in_configuration && current_configuration) {
                    current_configuration->libdirs.add(dir);
                } else {
                    out_data->libdirs.add(dir);
                }
            } else if (is_in_libs) {
                char lib_name[1024] = {};
                int i = 0;
                while (*line != '\n') {
                    if (*line == ' ') { line++; continue; }
                    lib_name[i] = *line;
                    line++;
                    i++;
                }
                Rsc_Dir lib = {};
                strcpy(lib.name, lib_name);
                if (is_in_configuration && current_configuration) {
                    current_configuration->libs.add(lib);
                } else {
                    out_data->libs.add(lib);
                }
            } else if (is_in_files) {
                char file_name[1024] = {};
                int i = 0;
                while (*line != '\n') {
                    if (*line == ' ') { line++; continue; }
                    file_name[i] = *line;
                    line++;
                    i++;
                }
                Rsc_Dir file = {};
                strcpy(file.name, file_name);
                out_data->files.add(file);
            }
        }
    }
}

static void write_compiler_line(const char *text, ...) {
    va_list args;

    char buf[1024] = {};
    
    va_start(args, text);
    vsprintf(buf, text, args);
    va_end(args);
    
    for (size_t i = 0; i < strlen(buf); ++i) {
        compiler_line.add(buf[i]);
    }
}

int main(int argc, char **argv) {
    if (argc == 1 || argc != 3) {
        printf("Usage: rsc <filename> <configuration>\n");
        return 1;
    }

    char directory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, directory);
    
    Rsc_Data data;
    parse_rsc_file(argv[1], &data);

    char *configuration = argv[2] + strlen("-configuration:");

    char *outputdir = "build";
    char *objdir = "obj";
    char *exename = "main";
    char *subsystem = "console";

    Array<Rsc_Dir> *includedirs = NULL;
    Array<Rsc_Dir> *libdirs = NULL;
    Array<Rsc_Dir> *libs = NULL;
    
    if (data.outputdir_set) outputdir = data.outputdir;
    if (data.objdir_set) objdir = data.objdir;
    if (data.exename_set) exename = data.exename;
    if (data.subsystem_set) subsystem = data.subsystem;

    if (data.includedirs_set) includedirs = &data.includedirs;
    if (data.libdirs_set) libdirs = &data.libdirs;
    if (data.libs_set) libs = &data.libs;

    bool optimizations = false;
    bool debug_symbols = false;

    Array<char *> defines;

#if _WIN32
    defines.add("OS_WINDOWS");
#endif
    
    for (int i = 0; i < data.configurations.count; ++i) {
        Configuration &current_data = data.configurations[i];
        if (strcmp(configuration, current_data.name) == 0) {
            if (current_data.outputdir_set) outputdir = current_data.outputdir;
            if (current_data.objdir_set) objdir = current_data.objdir;
            if (current_data.exename_set) exename = current_data.exename;
            if (current_data.subsystem_set) subsystem = current_data.subsystem;

            if (current_data.includedirs_set) includedirs = &current_data.includedirs;
            if (current_data.libdirs_set) libdirs = &current_data.libdirs;
            if (current_data.libs_set) libs = &current_data.libs;
            
            if (strcmp(current_data.name, "debug") == 0) debug_symbols = true;
            else if (strcmp(current_data.name, "release") == 0) optimizations = true;

            defines.add(to_upper(current_data.name));
        }
    }

    if (!outputdir) outputdir = "build";
    if (!exename) exename = "main";

    write_compiler_line("cl -nologo -Oi -FC ");

    if (optimizations) {
        write_compiler_line("-O2 ");
    }

    if (debug_symbols) {
        write_compiler_line("-Zi ");
    }

    for (int i = 0; i < defines.count; ++i) {
        write_compiler_line("-D%s ", defines[i]);
    }

    if (includedirs) {
        for (int i = 0; i < includedirs->count; ++i) {
            Rsc_Dir &includedir = includedirs->get(i);
            write_compiler_line("-I \"%s\\%s\" ", directory, includedir.name);
        }
    }

    if (!objdir) objdir = "obj";
    write_compiler_line("/Fo\"%s\\%s\\\\\" ", directory, objdir);
    
    for (int i = 0; i < data.files.count; ++i) {
        write_compiler_line("\"%s\\%s\" ", directory, data.files[i].name);
    }
    
    write_compiler_line("-link ");

    if (subsystem) {
        write_compiler_line("-subsystem:%s ", subsystem);
    }
    
    if (libdirs) {
        for (int i = 0; i < libdirs->count; ++i) {
            Rsc_Dir &libdir = libdirs->get(i);
            write_compiler_line("-LIBPATH:\"%s\" ", libdir.name);
        }
    }

    write_compiler_line("user32.lib gdi32.lib shell32.lib kernel32.lib winmm.lib ");
    
    if (libs) {
        for (int i = 0; i < libs->count; ++i) {
            Rsc_Dir &lib = libs->get(i);
            write_compiler_line("%s ", lib);
        }
    }

    write_compiler_line("-OUT:%s\\%s\\%s.exe", directory, outputdir, exename);
    write_compiler_line("\n");
    compiler_line.add(0);
    
    char command[256] = {};
    sprintf(command, "if not exist %s mkdir %s", outputdir, outputdir);
    system(command);
    sprintf(command, "if not exist %s mkdir %s", objdir, objdir);
    system(command);
    sprintf(command, "pushd %s", outputdir);
    system(command);
    system(compiler_line.data);
    sprintf(command, "popd", outputdir);
    system(command);

    return 0;
}
