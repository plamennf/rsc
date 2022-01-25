#include "core.h"
#include "array.h"

#ifdef OS_WINDOWS
#include "os_windows.cpp"
#endif

static Array<char> compiler_line(1024);
static Array<char> linker_line(1024);

struct Rsc_Dir {
    char *name = NULL;
    u64 last_write_time = 0;
};

struct Rsc_File {
    Array<Rsc_Dir> includes;
    
    char *name = NULL;
    u64 last_write_time = 0;
};

struct Configuration {
    char *name = NULL;

    char *outputdir = NULL;
    char *objdir = NULL;
    char *exename = NULL;

    Array<Rsc_Dir> includedirs;
    bool includedirs_set = false;
    Array<Rsc_Dir> libdirs;
    bool libdirs_set = false;
    Array<Rsc_Dir> libs;
    bool libs_set = false;

    char *subsystem = NULL;
    char *custom_compiler_line = NULL;
    char *custom_linker_line = NULL;
    char *prebuild_cmd = NULL;
    char *postbuild_cmd = NULL;
};

struct Rsc_Data {
    char *outputdir = NULL;
    char *objdir = NULL;
    char *exename = NULL;

    Array<Rsc_Dir> includedirs;
    bool includedirs_set = false;
    Array<Rsc_Dir> libdirs;
    bool libdirs_set = false;
    Array<Rsc_Dir> libs;
    bool libs_set = false;

    Array<Configuration> configurations;
    Array<Rsc_File *> files;

    char *subsystem = NULL;
    char *custom_compiler_line = NULL;
    char *custom_linker_line = NULL;
    char *prebuild_cmd = NULL;
    char *postbuild_cmd = NULL;
};

static char *std_files[] = {
    "assert.h",
    "ctype.h",
    "locale.h",
    "math.h",
    "setjmp.h",
    "signal.h",
    "stdarg.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "time.h",
    "float.h",
    "stdint.h",
    "stddef.h",
    "vector",
    "string",
    "unordered_map",
    "map",
};

static bool is_std_file(char *file_path) {
    bool result = false;
    for (umm i = 0; i < array_count(std_files); i++) {
        if (strings_match(file_path, std_files[i])) {
            result = true;
        }
    }
    return result;
}

static umm get_line_count(char *file_path) {
    umm result = 0;

    char *data = read_entire_file(file_path);
    defer { delete[] data; };

    char *at = data;
    for (;;) {
        char *line = consume_next_line(&at);
        if (!line) break;

        result++;
    }
    
    return result;
}

static bool find_string_in_array(const Array<char *> &array, char *string) {
    bool result = false;
    for (umm i = 0; i < array.count; i++) {
        if (strings_match(array[i], string)) {
            result = true;
        }
    }
    return result;
}

static char *find_path_of_include(Rsc_Data *data, char *file_path, char *included_in) {
    {
        char *char_loc = find_character_from_left(included_in, '/');
        if (!char_loc) {
            char_loc = find_character_from_left(included_in, '\\');
        }

        if (char_loc) {
            char *dir = copy_string(included_in);
            defer { delete[] dir; };
            dir[get_string_length(included_in) - get_string_length(char_loc)] = 0;
                //dir += 1;

            char full_path[1024] = {};
            stbsp_snprintf(full_path, sizeof(full_path), "%s/%s", dir, file_path);
            
            if (file_exists(full_path)) {
                return copy_string(full_path);
            }
        }
    }

    // TODO: Move this to when add the files to the compiler line, because this relies on the includedirs to be set.
    {
        for (umm i = 0; i < data->includedirs.count; i++) {
            char *dir = data->includedirs[i].name;
            char full_path[1024] = {};
            stbsp_snprintf(full_path, sizeof(full_path), "%s/%s", dir, file_path);

            if (file_exists(full_path)) {
                return copy_string(full_path);
            }
        }
    }

    return NULL;
}

static void parse_file(Rsc_Data *data, Rsc_File *file) {
    char *file_data = read_entire_file(file->name);
    if (!file_data) return;
    defer { delete[] file_data; };

    char *at = file_data;
    while (at = strstr(at, "#include")) {
        char *line = consume_next_line(&at);
        line += get_string_length("#include");
        line = eat_spaces(line);
        line = eat_trailing_spaces(line);

        line += 1;

        umm line_length = get_string_length(line);
        char *include = new char[line_length];
        memset(include, 0, line_length);

        for (umm i = 0; i < line_length; i++) {
            if (line[i] == '"' || line[i] == '>') break;
            include[i] = line[i];
        }

        if (is_std_file(include)) {
            delete include;
            continue;
        }

        Rsc_Dir dir = {};
        char *path = find_path_of_include(data, include, file->name);
        if (!path) return;
        dir.name = path;
        dir.last_write_time = get_last_write_time(dir.name);
        file->includes.add(dir);
    }
}

static Rsc_Data *parse_rsc_file(char *file_path) {
    char *data = read_entire_file(file_path);
    if (!data) return NULL;
    defer { delete[] data; };

    Rsc_Data *out_data = new Rsc_Data();

    char *at = data;
    bool is_in_configuration = false;
    Configuration *current_configuration = NULL;
    bool is_in_includedirs = false;
    bool is_in_libdirs = false;
    bool is_in_libs = false;
    bool is_in_files = false;
    for (;;) {
        char *line = consume_next_line(&at);
        if (!line) break;

        if (strstr(line, "outputdir")) {
            char *cursor = strstr(line, "outputdir");
            cursor += get_string_length("outputdir");

            cursor = eat_spaces(cursor);
            cursor = eat_trailing_spaces(cursor);
            
            if (is_in_configuration && current_configuration) {
                current_configuration->outputdir = copy_string(cursor);
            } else {
                out_data->outputdir = copy_string(cursor);
            }
        } else if (strstr(line, "objdir")) {
            char *cursor = strstr(line, "objdir");
            cursor += get_string_length("objdir");

            cursor = eat_spaces(cursor);
            cursor = eat_trailing_spaces(cursor);
            
            if (is_in_configuration && current_configuration) {
                current_configuration->objdir = copy_string(cursor);
            } else {
                out_data->objdir = copy_string(cursor);
            }
        } else if (strstr(line, "exename")) {
            char *cursor = strstr(line, "exename");
            cursor += get_string_length("exename");

            cursor = eat_spaces(cursor);
            cursor = eat_trailing_spaces(cursor);
            
            if (is_in_configuration && current_configuration) {
                current_configuration->exename = copy_string(cursor);
            } else {
                out_data->exename = copy_string(cursor);
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
        } else if (strstr(line, "customcompilerline")) {
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            line += get_string_length("customcompilerline");
            line = eat_spaces(line);
            line += 1;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            if (is_in_configuration) {
                current_configuration->custom_compiler_line = copy_string(line);
            } else {
                out_data->custom_compiler_line = copy_string(line);
            }
        } else if (strstr(line, "customlinkerline")) {
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            line += get_string_length("customlinkerline");
            line = eat_spaces(line);
            line += 1;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            if (is_in_configuration) {
                current_configuration->custom_linker_line = copy_string(line);
            } else {
                out_data->custom_linker_line = copy_string(line);
            }
        } else if (strstr(line, "prebuildcmd")) {
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            line += get_string_length("prebuildcmd");
            line = eat_spaces(line);
            line += 1;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            if (is_in_configuration) {
                current_configuration->prebuild_cmd = copy_string(line);
            } else {
                out_data->prebuild_cmd = copy_string(line);
            }
        } else if (strstr(line, "postbuildcmd")) {
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            line += get_string_length("postbuildcmd");
            line = eat_spaces(line);
            line += 1;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            if (is_in_configuration) {
                current_configuration->postbuild_cmd = copy_string(line);
            } else {
                out_data->postbuild_cmd = copy_string(line);
            }
        } else {
            if (is_in_configuration) {
                out_data->configurations.emplace();
                current_configuration = &out_data->configurations[out_data->configurations.count - 1];
                umm line_length = get_string_length(line);
                current_configuration->name = new char[line_length + 1];
                memset(current_configuration->name, 0, line_length + 1);
                int i = 0;
                while (*line != ':') {
                    if (*line == ' ') {
                        line++;
                        continue;
                    }
                    current_configuration->name[i] = *line;
                    line++;
                    i++;
                }
            } else if (is_in_includedirs) {
                Rsc_Dir dir = {};
                line = eat_spaces(line);
                line = eat_trailing_spaces(line);
                dir.name = copy_string(line);
                if (is_in_configuration && current_configuration) {
                    current_configuration->includedirs.add(dir);
                } else {
                    out_data->includedirs.add(dir);
                }
            } else if (is_in_libdirs) {
                Rsc_Dir dir = {};
                line = eat_spaces(line);
                line = eat_trailing_spaces(line);
                dir.name = copy_string(line);
                if (is_in_configuration && current_configuration) {
                    current_configuration->libdirs.add(dir);
                } else {
                    out_data->libdirs.add(dir);
                }
            } else if (is_in_libs) {
                Rsc_Dir dir = {};
                line = eat_spaces(line);
                line = eat_trailing_spaces(line);
                dir.name = copy_string(line);
                if (is_in_configuration && current_configuration) {
                    current_configuration->libs.add(dir);
                } else {
                    out_data->libs.add(dir);
                }
            } else if (is_in_files) {
                Rsc_File *file = new Rsc_File;
                line = eat_spaces(line);
                line = eat_trailing_spaces(line);
                file->name = copy_string(line);
                file->last_write_time = get_last_write_time(file->name);
                out_data->files.add(file);
            }
        }
    }
    
    return out_data;
}

static void write_compiler_line(char *text, ...) {
    va_list args;

    char buffer[1024] = {};

    va_start(args, text);
    stbsp_vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);

    for (umm i = 0; i < get_string_length(buffer); i++) {
        compiler_line.add(buffer[i]);
    }
}

static void write_linker_line(char *text, ...) {
    va_list args;

    char buffer[1024] = {};

    va_start(args, text);
    stbsp_vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);

    for (umm i = 0; i < get_string_length(buffer); i++) {
        linker_line.add(buffer[i]);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file.rsc> <configuration> <-B>\n", argv[0]);
        fflush(stderr);
        return 1;
    }

    double rsc_start_time = get_time();
    
    Rsc_Data *data = parse_rsc_file(argv[1]);
    if (!data) {
        fprintf(stderr, "Unable to open file '%s' for reading!\n", argv[1]);
        fflush(stderr);
        return 1;
    }

    bool rebuild_all_files = false;
    if (argc == 4) {
        if (strings_match(argv[3], "-B")) {
            rebuild_all_files = true;
        }
    }
    
    char *directory = get_current_directory();

    char *configuration = argv[2] + get_string_length("-configuration:");
    configuration = to_upper(configuration);
    
    char *outputdir = "bin";
    char *objdir = "bin-int";
    char *exename = "main";
    char *subsystem = "console";
    char *custom_compiler_line = NULL;
    char *custom_linker_line = NULL;
    char *prebuild_cmd = NULL;
    char *postbuild_cmd = NULL;

    Array<Rsc_Dir> includedirs;
    Array<Rsc_Dir> libdirs;
    Array<Rsc_Dir> libs;

    if (data->outputdir) outputdir = data->outputdir;
    if (data->objdir) objdir = data->objdir;
    if (data->exename) exename = data->exename;
    if (data->subsystem) subsystem = data->subsystem;
    if (data->custom_compiler_line) custom_compiler_line = data->custom_compiler_line;
    if (data->custom_linker_line) custom_linker_line = data->custom_linker_line;
    if (data->prebuild_cmd) prebuild_cmd = data->prebuild_cmd;
    if (data->postbuild_cmd) postbuild_cmd = data->postbuild_cmd;
    
    if (data->includedirs_set) {
        includedirs.add(data->includedirs);
    }
    if (data->libdirs_set) {
        libdirs.add(data->libdirs);
    }
    if (data->libs_set) {
        libs.add(data->libs);
    }

    bool optimizations = false;
    bool debug_symbols = false;
    
    Array<char *> defines;
    
#ifdef OS_WINDOWS
    defines.add("OS_WINDOWS");
#endif

    for (umm i = 0; i < data->configurations.count; i++) {
        Configuration &current_data = data->configurations[i];
        if (strings_match(configuration, to_upper(current_data.name))) {
            if (current_data.outputdir) outputdir = current_data.outputdir;
            if (current_data.objdir) objdir = current_data.objdir;
            if (current_data.exename) exename = current_data.exename;
            if (current_data.subsystem) subsystem = current_data.subsystem;
            if (current_data.custom_compiler_line) {
                if (custom_compiler_line) {
                    custom_compiler_line = concatenate(custom_compiler_line, current_data.custom_compiler_line);
                } else {
                    custom_compiler_line = current_data.custom_compiler_line;
                }
            }
            if (current_data.custom_linker_line) {
                if (custom_linker_line) {
                    custom_linker_line = concatenate(custom_linker_line, current_data.custom_linker_line);
                } else {
                    custom_linker_line = current_data.custom_linker_line;
                }
            }
            if (current_data.prebuild_cmd) {
                if (prebuild_cmd) {
                    prebuild_cmd = concatenate(prebuild_cmd, current_data.prebuild_cmd);
                } else {
                    prebuild_cmd = current_data.prebuild_cmd;
                }
            }
            if (current_data.postbuild_cmd) {
                if (postbuild_cmd) {
                    postbuild_cmd = concatenate(postbuild_cmd, current_data.postbuild_cmd);
                } else {
                    postbuild_cmd = current_data.postbuild_cmd;
                }
            }
            
            if (current_data.includedirs_set) {
                if (!includedirs.allocated) {
                    libdirs.resize(32);
                }
                for (umm j = 0; j < current_data.includedirs.count; j++) {
                    includedirs.add(current_data.includedirs[j]);
                }
            }
            
            if (current_data.libdirs_set) {
                if (!libdirs.allocated) {
                    libdirs.resize(32);
                }
                for (umm j = 0; j < current_data.libdirs.count; j++) {
                    libdirs.add(current_data.libdirs[j]);
                }
            }

            if (current_data.libs_set) {
                if (!libs.allocated) {
                    libs.resize(32);
                }
                for (umm j = 0; j < current_data.libs.count; j++) {
                    libs.add(current_data.libs[j]);
                }
            }

            if (strings_match(current_data.name, "debug")) debug_symbols = true;
            else if (strings_match(current_data.name, "release")) optimizations = true;

            defines.add(copy_string(to_upper(current_data.name)));

            break;
        }
    }

    char output_path[1024] = {};
    stbsp_snprintf(output_path, sizeof(output_path), "%s\\%s\\%s.exe", directory, outputdir, exename);

    u64 exe_last_write_time = get_last_write_time(output_path);

    if (prebuild_cmd) {
        system(prebuild_cmd);
    }
    
    write_compiler_line("cl -c -nologo -Oi -FC -EHsc -fp:fast -fp:except- ");

    if (optimizations) {
        write_compiler_line("-02 -Ob2 -MT ");
    } else {
        write_compiler_line("-Od -Ob0 -MTd ");
    }

    if (debug_symbols) {
        write_compiler_line("-Zi ");
    }

    for (umm i = 0; i < defines.count; i++) {
        write_compiler_line("-D%s ", defines[i]);
    }

    for (umm i = 0; i < includedirs.count; i++) {
        Rsc_Dir &includedir = includedirs[i];
        write_compiler_line("-I \"%s\\%s\" ", directory, includedir.name);
    }

    write_compiler_line("/Fo\"%s\\%s\\\\\" ", directory, objdir);

    if (custom_compiler_line) {
        write_compiler_line(" %s ", custom_compiler_line);
    }

    Array<char *> counted_files;
    umm total_line_count = 0;
    
    bool has_files_to_compile = false;
    if (rebuild_all_files) {
        has_files_to_compile = true;

        for (umm i = 0; i < data->files.count; i++) {
            Rsc_File *file = data->files[i];

            parse_file(data, file);
                        
            total_line_count += get_line_count(file->name);
            counted_files.add(file->name);

            for (umm j = 0; j < file->includes.count; j++) {
                Rsc_Dir include = file->includes[j];
                if (!find_string_in_array(counted_files, include.name)) {
                    total_line_count += get_line_count(include.name);
                }
            }

            write_compiler_line("\"%s\\%s\" ", directory, file->name);
        }
    } else {
        for (umm i = 0; i < data->files.count; i++) {
            Rsc_File *file = data->files[i];

            parse_file(data, file);
        
            total_line_count += get_line_count(file->name);
            counted_files.add(file->name);

            for (umm j = 0; j < file->includes.count; j++) {
                Rsc_Dir include = file->includes[j];
                if (!find_string_in_array(counted_files, include.name)) {
                    total_line_count += get_line_count(include.name);
                }
            }
            
            bool add_file_to_compile_list = false;
            if (file->last_write_time < exe_last_write_time) {
                for (umm j = 0; j < file->includes.count; j++) {
                    Rsc_Dir include = file->includes[j];
                    if (include.last_write_time > exe_last_write_time) {
                        add_file_to_compile_list = true;
                        has_files_to_compile = true;
                        break;
                    }
                }
            } else {
                add_file_to_compile_list = true;
                has_files_to_compile = true;
            }
        
            if (add_file_to_compile_list) {
                write_compiler_line("\"%s\\%s\" ", directory, file->name);
            }
        }
    }
    
    write_linker_line("link /nologo ");

    for (umm i = 0; i < data->files.count; i++) {
        Rsc_File *file = data->files[i];

        char *name = file->name; // Already used the file->name so we can change it.

        char *char_loc = find_character_from_left(name, '/');
        if (!char_loc) {
            char_loc = find_character_from_left(name, '\\');
        }

        char_loc += 1;

        name = char_loc;
        
        char_loc = find_character_from_right(name, '.');

        char *thing = name;
        thing[get_string_length(name) - get_string_length(char_loc)] = 0;
        
        char path[1024] = {};
        stbsp_snprintf(path, sizeof(path), "%s\\%s\\%s.obj", directory, objdir, thing);
        write_linker_line("%s ", path);
    }
    
    write_linker_line("-subsystem:%s ", subsystem);

    for (umm i = 0; i < libdirs.count; i++) {
        Rsc_Dir &libdir = libdirs[i];
        write_linker_line("-LIBPATH:\"%s\" ", libdir.name);
    }

    write_linker_line("user32.lib gdi32.lib shell32.lib kernel32.lib winmm.lib ");

    for (umm i = 0; i < libs.count; i++) {
        Rsc_Dir &lib = libs[i];
        write_linker_line("%s ", lib.name);
    }

    write_linker_line("-OUT:%s ", output_path);
    write_linker_line("\n");

    if (custom_linker_line) {
        write_linker_line(" %s ", custom_linker_line);
    }
    
    compiler_line.add(0);
    linker_line.add(0);

    printf("Line count(including blank lines and comments and included headers(without crt)): %llu\n", total_line_count);
    fflush(stdout);
    
    if (has_files_to_compile) {
        char command[1024] = {};
        stbsp_sprintf(command, "@echo off");
        system(command);
        stbsp_sprintf(command, "if not exist %s mkdir %s", outputdir, outputdir);
        system(command);
        stbsp_sprintf(command, "if not exist %s mkdir %s", objdir, objdir);
        system(command);
        stbsp_sprintf(command, "pushd %s", outputdir);
        system(command);
        double rsc_end_time = get_time();
        double rsc_elapsed = rsc_end_time - rsc_start_time;
        double msvc_start_time = get_time();
        system(compiler_line.data);
        double msvc_end_time = get_time();
        double msvc_elapsed = msvc_end_time - msvc_start_time;
        double linker_start_time = get_time();
        system(linker_line.data);
        double linker_end_time = get_time();
        double linker_elapsed = linker_end_time - linker_start_time;
        stbsp_sprintf(command, "popd", outputdir);
        system(command);

        printf("Total time: %.4f\n", rsc_elapsed + msvc_elapsed + linker_elapsed);
        printf("RSC time: %.4f\n", rsc_elapsed);
        printf("MSVC time: %.4f\n", msvc_elapsed);
        printf("Linker time: %.4f\n", linker_elapsed);
        fflush(stdout);
    }

    if (postbuild_cmd) {
        system(postbuild_cmd);
    }
        
    return 0;
}
