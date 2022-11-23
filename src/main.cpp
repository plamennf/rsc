#include "general.h"
#include "ds.h"
#include "text_file_handler.h"
#include "os.h"

#include <stdio.h>

double rsc_start_time = 0.0;

enum Output_Type {
    OUTPUT_UNKNOWN,
    OUTPUT_CONSOLE_APP,
    OUTPUT_WINDOWED_APP,
    OUTPUT_STATIC_LIB,
};

enum Runtime_Type {
    RUNTIME_DEBUG,
    RUNTIME_RELEASE,
};

struct Configuration {
    char *name = NULL;
    char *lowercased_name = NULL;
    
    char *outputdir = NULL;
    char *objdir = NULL;
    char *outputname = NULL;
    
    Array <char *> includedirs;
    Array <char *> libdirs;
    Array <char *> libs;
    
    Array <char *> defines;
    
    Output_Type type = OUTPUT_UNKNOWN;

    bool static_runtime = false;
    bool static_runtime_set = false;
    bool debug_symbols = false;
    bool debug_symbols_set = false;
    bool optimize = false;
    bool optimize_set = false;

    Runtime_Type runtime_type = RUNTIME_DEBUG;
    
    Array <char *> cfiles;

    char *pchheader = NULL;
    char *pchsource = NULL;
};

struct Rsc_Project {
    Array <Configuration *> configurations;

    char *name = NULL;
    char *lowercased_name = NULL;
    
    char *outputdir = NULL;
    bool outputdir_set = false;
    char *objdir = NULL;
    bool objdir_set = false;
    char *outputname = NULL;
    bool outputname_set = false;
    
    Output_Type type = OUTPUT_UNKNOWN;

    bool static_runtime = false;
    bool static_runtime_set = false;
    bool debug_symbols = false;
    bool debug_symbols_set = false;
    bool optimize = false;
    bool optimize_set = false;

    Array <char *> includedirs;
    Array <char *> libdirs;
    Array <char *> libs;

    Array <char *> defines;
    
    Runtime_Type runtime_type = RUNTIME_DEBUG;
    
    Array <char *> cfiles;

    char *pchheader = NULL;
    char *pchsource = NULL;
};

struct Rsc_Data {
    bool should_rebuild_all;
    
    Array <Configuration *> configurations;
    Array <Rsc_Project *> projects;

    void parse_file(char *filepath);
};

struct Command_Line_Arguments {
    char *rsc_file = NULL;
    char *configuration = NULL;
    bool rebuild_all = false;
};

static char *do_macro_substitutions(char *s, Rsc_Project *project, Configuration *configuration) {
    Array <char> sanitized_s;
    for (char *at = s; *at;) {
        if (at[0] == '%') {
            if (at[1] != '{') {
                log_error("Macros always start with %{ and end with }\n");
                exit(1);
            }
            at += 2;

            Array <char> macro_name;
            while (1) {
                if (at[0] == 0) {
                    log_error("EOF found while parsing string.\n");
                    exit(1);
                }
                if (at[0] == '}') break;

                macro_name.add(at[0]);
                at++;
            }
            at++;
            macro_name.add(0);

            if (strings_match(macro_name.data, "ProjectName")) {
                sanitized_s.add(project->name, get_string_length(project->name));
            } else if (strings_match(macro_name.data, "Configuration")) {
                sanitized_s.add(configuration->name, get_string_length(configuration->name));
            } else if (strings_match(macro_name.data, "OutputDir")) {
                char *outputdir = sprint("build\\%s", configuration->name);
                if (project->outputdir) outputdir = project->outputdir;
                if (configuration->outputdir) outputdir = configuration->outputdir;
                replace_forwardslash_with_backslash(outputdir);
                outputdir = do_macro_substitutions(outputdir, project, configuration);
                sanitized_s.add(outputdir, get_string_length(outputdir));
            } else {
                log_error("Invalid value for macro. Valid values are:\n");
                log_error("   ProjectName\n");
                log_error("   Configuration\n");
                log_error("   OutputDir\n");
                exit(1);
            }
        } else {
            sanitized_s.add(at[0]);
            at++;
        }
    }
    sanitized_s.add(0);
    return sanitized_s.copy_to_array();
}

static Command_Line_Arguments parse_command_line_arguments(int argc, char **argv) {
    Command_Line_Arguments result;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (starts_with(arg, "-configuration:")) {
            arg += get_string_length("-configuration:");
            
            result.configuration = temp_copy_string(arg);
        } else if (strings_match(arg, "-B")) {
            result.rebuild_all = true;
        } else {
            if (result.rsc_file) {
                log("ERROR: More than one .rsc files have been provided.\n");
                continue;
            }
            result.rsc_file = temp_copy_string(arg);
        }
    }
    
    return result;
}

static void print_usage() {
    log("Usage: rsc.exe <file.rsc> -configuration:<Configuration> <Optional Flags>\n");
    log("Optional Flags:\n");
    log("    -B -> Rebuild all\n");
}

void Rsc_Data::parse_file(char *filepath) {
    Text_File_Handler handler;
    handler.start_file(filepath, filepath, "rsc");

    bool is_in_configurations = false;
    Rsc_Project *current_project = NULL;
    bool is_in_project = false;
    bool is_in_cfiles = false;
    Configuration *current_configuration = NULL;
    bool is_in_configuration_filter = false;
    bool is_in_includedirs = false;
    bool is_in_libdirs = false;
    bool is_in_libs = false;
    bool is_in_defines = false;
    while (1) {
        char *line = handler.consume_next_line();
        if (!line) break;
        
        if (starts_with(line, "configurations")) {
            if (is_in_configurations) {
                handler.report_error("configurations already exists\n");
                exit(1);
            }
            is_in_configurations = true;
            is_in_cfiles = false;
            is_in_project = false;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = false;
            is_in_defines = false;
        } else if (starts_with(line, "project")) {
            is_in_cfiles = false;
            is_in_configurations = false;
            is_in_project = true;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = false;
            is_in_configuration_filter = false;
            is_in_defines = false;
            
            line += get_string_length("project");
            line = eat_spaces(line);

            Rsc_Project *project = new Rsc_Project();//GetStruct(Rsc_Project);
            project->name = temp_copy_string(line);
            project->lowercased_name = temp_copy_string(lowercase(line));
            projects.add(project);
            
            current_project = project;
        } else if (starts_with(line, "type")) {
            if (!is_in_project) {
                handler.report_error("type can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("type");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after type: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> type_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                type_string.add(line[0]);
                line++;
            }
            type_string.add(0);

            Output_Type type = OUTPUT_UNKNOWN;
            if (strings_match(type_string.data, "ConsoleApp")) {
                type = OUTPUT_CONSOLE_APP;
            } else if (strings_match(type_string.data, "WindowedApp")) {
                type = OUTPUT_WINDOWED_APP;
            } else if (strings_match(type_string.data, "StaticLib")) {
                type = OUTPUT_STATIC_LIB;
            } else {
                handler.report_error("Invalid value for type. Valid values are:");
                handler.report_error("   ConsoleApp\n");
                exit(1);
            }

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->type = type;
            } else {
                current_project->type = type;
            }
        } else if (starts_with(line, "pchheader")) {
            if (!is_in_project) {
                handler.report_error("pchheader can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("pchheader");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after pchheader: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> type_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                type_string.add(line[0]);
                line++;
            }
            type_string.add(0);

            if (strchr(type_string.data, '/') || strchr(type_string.data, '\\')) {
                handler.report_error("pchheader should contain only the header name not its directory");
                exit(1);
            }
            
            if (is_in_configuration_filter && current_configuration) {
                current_configuration->pchheader = copy_string(type_string.data);
            } else {
                current_project->pchheader = copy_string(type_string.data);
            }
        } else if (starts_with(line, "pchsource")) {
            if (!is_in_project) {
                handler.report_error("pchsource can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("pchsource");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after pchsource: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> type_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                type_string.add(line[0]);
                line++;
            }
            type_string.add(0);

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->pchsource = copy_string(type_string.data);
            } else {
                current_project->pchsource = copy_string(type_string.data);
            }
        } else if (starts_with(line, "outputdir")) {
            if (!is_in_project) {
                handler.report_error("outputdir can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("outputdir");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after outputdir: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> outputdir_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                outputdir_string.add(line[0]);
                line++;
            }
            outputdir_string.add(0);

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->outputdir = temp_copy_string(outputdir_string.data);
            } else {
                current_project->outputdir = temp_copy_string(outputdir_string.data);
            }
        } else if (starts_with(line, "objdir")) {
            if (!is_in_project) {
                handler.report_error("objdir can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("objdir");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after objdir: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> objdir_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                objdir_string.add(line[0]);
                line++;
            }
            objdir_string.add(0);

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->objdir = temp_copy_string(objdir_string.data);
            } else {
                current_project->objdir = temp_copy_string(objdir_string.data);
            }
        } else if (starts_with(line, "outputname")) {
            if (!is_in_project) {
                handler.report_error("outputname can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }
            
            line += get_string_length("outputname");

            line = eat_spaces(line);
            if (line[0] != '"') {
                handler.report_error("Expected string after outputname: String are always wrapped in "".");
                exit(1);
            }
            line++;
            
            Array <char> outputname_string;
            while (1) {
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                outputname_string.add(line[0]);
                line++;
            }
            outputname_string.add(0);
            
            if (is_in_configuration_filter && current_configuration) {
                current_configuration->outputname = temp_copy_string(outputname_string.data);
            } else {
                current_project->outputname = temp_copy_string(outputname_string.data);
            }
        } else if (starts_with(line, "cfiles")) {
            is_in_cfiles = true;
            is_in_configurations = false;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = false;
            is_in_defines = false;
        } else if (starts_with(line, "defines")) {
            is_in_cfiles = false;
            is_in_configurations = false;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = false;
            is_in_defines = true;
        } else if (starts_with(line, "}")) {
            
        } else if (starts_with(line, "runtime")) {
            if (!is_in_project) {
                handler.report_error("runtime can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }

            line += get_string_length("runtime");
            line = eat_spaces(line);

            if (strings_match(line, "\"Debug\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->runtime_type = RUNTIME_DEBUG;
                } else {
                    current_project->runtime_type = RUNTIME_DEBUG;
                }
            } else if (strings_match(line, "\"Release\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->runtime_type = RUNTIME_RELEASE;
                } else {
                    current_project->runtime_type = RUNTIME_RELEASE;
                }
            } else {
                handler.report_error("Invalid value for runtime. Valid values are Debug and Release");
                exit(1);
            }
        } else if (starts_with(line, "staticruntime")) {
            if (!is_in_project) {
                handler.report_error("staticruntime can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }

            line += get_string_length("staticruntime");
            line = eat_spaces(line);

            lowercase(line);

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->static_runtime_set = true;
            } else {
                current_project->static_runtime_set = true;
            }
            if (strings_match(line, "\"on\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->static_runtime = true;
                } else {
                    current_project->static_runtime = true;
                }
            } else if (strings_match(line, "\"off\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->static_runtime = false;
                } else {
                    current_project->static_runtime = false;
                }
            } else {
                handler.report_error("Invalid value for staticruntime. Valid values are on and off");
                exit(1);
            }
        } else if (starts_with(line, "debugsymbols")) {
            if (!is_in_project) {
                handler.report_error("debugsymbols can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }

            line += get_string_length("debugsymbols");
            line = eat_spaces(line);

            lowercase(line);


            if (is_in_configuration_filter && current_configuration) {
                current_configuration->debug_symbols_set = true;
            } else {
                current_project->debug_symbols_set = true;
            }
            
            if (strings_match(line, "\"on\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->debug_symbols = true;
                } else {
                    current_project->debug_symbols = true;
                }
            } else if (strings_match(line, "\"off\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->debug_symbols = false;
                } else {
                    current_project->debug_symbols = false;
                }
            } else {
                handler.report_error("Invalid value for debugsymbols. Valid values are on and off");
                exit(1);
            }
        } else if (starts_with(line, "optimize")) {
            if (!is_in_project) {
                handler.report_error("optimize can only be set for project.");
                exit(1);
            }
            if (!current_project) {
                handler.report_error("Internal error: current_project is NULL");
                exit(1);
            }

            line += get_string_length("optimize");
            line = eat_spaces(line);

            lowercase(line);

            if (is_in_configuration_filter && current_configuration) {
                current_configuration->optimize_set = true;
            } else {
                current_project->optimize_set = true;
            }
            
            if (strings_match(line, "\"on\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->optimize = true;
                } else {
                    current_project->optimize = true;
                }
            } else if (strings_match(line, "\"off\"")) {
                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->optimize = false;
                } else {
                    current_project->optimize = false;
                }
            } else {
                handler.report_error("Invalid value for optimize. Valid values are on and off");
                exit(1);
            }
        } else if (starts_with(line, "filter")) {
            line += get_string_length("filter");
            line = eat_spaces(line);

            if (line[0] != '"') {
                handler.report_error("Strings should be enclosed in "".");
                exit(1);
            }
            line++;

            Array <char> filter_name;
            while (1) {                
                if (line[0] == 0) {
                    handler.report_error("EOF found while parsing string.");
                    exit(1);
                }
                if (line[0] == '"') break;

                filter_name.add(line[0]);
                line++;
            }
            line++;
            filter_name.add(0);

            char *filter = filter_name.data;

            if (starts_with(filter, "configuration:")) {
                filter += get_string_length("configuration:");
                is_in_configuration_filter = true;

                lowercase(filter);

                for (int i = 0; i < configurations.count; i++) {
                    Configuration *cfg = configurations[i];
                    if (strings_match(cfg->lowercased_name, filter)) {
                        current_configuration = cfg;
                    }
                }
                if (!current_configuration) {
                    handler.report_error("configuration wanted for filter does not exist");
                    exit(1);
                }
            }
        } else if (starts_with(line, "includedirs")) {
            is_in_cfiles = false;
            is_in_configurations = false;
            is_in_project = true;
            is_in_includedirs = true;
            is_in_libdirs = false;
            is_in_libs = false;
        } else if (starts_with(line, "libdirs")) {
            is_in_cfiles = false;
            is_in_configurations = false;
            is_in_project = true;
            is_in_includedirs = false;
            is_in_libdirs = true;
            is_in_libs = false;
        } else if (starts_with(line, "libs")) {
            is_in_cfiles = false;
            is_in_configurations = false;
            is_in_project = true;
            is_in_includedirs = false;
            is_in_libdirs = false;
            is_in_libs = true;
        } else {
            if (is_in_configurations) {
                Configuration *configuration = new Configuration();
                configuration->name = temp_copy_string(line);
                configuration->lowercased_name = temp_copy_string(lowercase(line));
                
                configurations.add(configuration);
            } else if (is_in_cfiles) {
                if (line[0] == '{') line++;
                line = eat_spaces(line);
                if (line[0] == 0) continue;

                if (!is_in_project) {
                    handler.report_error("cfiles can only be set for project.");
                    exit(1);
                }
                if (!current_project) {
                    handler.report_error("Internal error: current_project is NULL");
                    exit(1);
                }
                
                current_project->cfiles.add(temp_copy_string(line));
            } else if (is_in_includedirs) {
                if (line[0] == '{') line++;
                line = eat_spaces(line);
                if (line[0] == 0) continue;

                if (!is_in_project) {
                    handler.report_error("includedirs can only be set for project.");
                    exit(1);
                }
                if (!current_project) {
                    handler.report_error("Internal error: current_project is NULL");
                    exit(1);
                }

                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->includedirs.add(temp_copy_string(line));
                } else {
                    current_project->includedirs.add(temp_copy_string(line));
                }
            } else if (is_in_libdirs) {
                if (line[0] == '{') line++;
                line = eat_spaces(line);
                if (line[0] == 0) continue;

                if (!is_in_project) {
                    handler.report_error("includedirs can only be set for project.");
                    exit(1);
                }
                if (!current_project) {
                    handler.report_error("Internal error: current_project is NULL");
                    exit(1);
                }

                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->libdirs.add(temp_copy_string(line));
                } else {
                    current_project->libdirs.add(temp_copy_string(line));
                }
            } else if (is_in_libs) {
                if (line[0] == '{') line++;
                line = eat_spaces(line);
                if (line[0] == 0) continue;

                if (!is_in_project) {
                    handler.report_error("includedirs can only be set for project.");
                    exit(1);
                }
                if (!current_project) {
                    handler.report_error("Internal error: current_project is NULL");
                    exit(1);
                }

                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->libs.add(temp_copy_string(line));
                } else {
                    current_project->libs.add(temp_copy_string(line));
                }
            } else if (is_in_defines) {
                if (line[0] == '{') line++;
                line = eat_spaces(line);
                if (line[0] == 0) continue;

                if (!is_in_project) {
                    handler.report_error("defines can only be set for project.");
                    exit(1);
                }
                if (!current_project) {
                    handler.report_error("Internal error: current_project is NULL");
                    exit(1);
                }

                if (is_in_configuration_filter && current_configuration) {
                    current_configuration->defines.add(temp_copy_string(line));
                } else {
                    current_project->defines.add(temp_copy_string(line));
                }
            }
        }
    }
}

static char *get_directory_from_filename(char *string) {
    if (!string) return NULL;

    char *result = temp_copy_string(string);
    
    char *slash = strrchr(result, '/');
    result[slash - result] = 0;
    return result;
}

static void check_file_for_includes(char *filename, Array <char *> &includes) {
    char *data = os_read_entire_file(filename);
    if (!data) return;
    defer { delete [] data; };

    char *at = data;
    while (1) {
        char *line = consume_next_line(&at);
        if (!line) break;

        line = eat_spaces(line);

        if (!starts_with(line, "#include")) continue;
            
        line += get_string_length("#include");
        line = eat_spaces(line);

        if (line[0] != '"' && line[0] != '<') {
            //log_error("Shit happened\n");
            continue;
        }

        line++;
        
        Array <char> includename_string;
        while (1) {
            if (line[0] == 0) {
                log_error("EOF found while parsing string in c file.");
                continue;
            }
            if (line[0] == '>' || line[0] == '"') break;

            includename_string.add(line[0]);
            line++;
        }
        includename_string.add(0);

        char *filename_directory = get_directory_from_filename(filename);
        
        Array <char *> directories;
        directories.add(filename_directory);
        
        bool file_exists = false;
        char *full_path = NULL;
        for (int i = 0; i < directories.count; i++) {
            char *dir = directories[i];
            if (!dir) {
                file_exists = os_file_exists(filename);
                continue;
            }

            char *path = sprint("%s/%s", dir, includename_string.data);
            file_exists = os_file_exists(path);
            if (file_exists) {
                full_path = path;
            }
        }

        if (file_exists) {
            includes.add(full_path);
            check_file_for_includes(full_path, includes);
        }
    }
}

static void execute_msvc_for_project(Rsc_Data *data, Rsc_Project *project, Configuration *configuration, u64 rsc_modtime) {
    char *outputdir = sprint("build\\%s", configuration->name);
    if (project->outputdir) outputdir = project->outputdir;
    if (configuration->outputdir) outputdir = configuration->outputdir;
    replace_forwardslash_with_backslash(outputdir);
    outputdir = do_macro_substitutions(outputdir, project, configuration); // @Leak
    
    char *objdir = sprint("obj\\%s\\%s", project->name, configuration->name);
    if (project->objdir) objdir = project->objdir;
    if (configuration->objdir) objdir = configuration->objdir;
    replace_forwardslash_with_backslash(objdir);
    objdir = do_macro_substitutions(objdir, project, configuration); // @Leak

    char *outputname = project->name;
    if (project->outputname) outputname = project->outputname;
    if (configuration->outputname) outputname = configuration->outputname;
    replace_forwardslash_with_backslash(outputname);
    outputname = do_macro_substitutions(outputname, project, configuration); // @Leak
    
    u64 exe_modtime = 0;
    char *exepath = sprint("%s/%s.exe", outputdir, outputname);
    os_get_last_write_time(exepath, &exe_modtime);

    char *pchheader = project->pchheader;
    if (configuration->pchheader) pchheader = configuration->pchheader;
    pchheader = replace_forwardslash_with_backslash(pchheader);

    char *pchsource = project->pchsource;
    if (configuration->pchsource) pchsource = configuration->pchsource;
    pchsource = replace_forwardslash_with_backslash(pchsource);
    
    if (rsc_modtime > exe_modtime) {
        data->should_rebuild_all = true;
    }

    os_make_directory_if_not_exist(outputdir);
    os_make_directory_if_not_exist(objdir);
    
    Array <char *> files_to_compile;
    for (int i = 0; i < project->cfiles.count; i++) {
        char *filename = project->cfiles[i];

        if (data->should_rebuild_all) {
            files_to_compile.add(filename);
            continue;
        }
        
        Array <char *> file_includes;
        check_file_for_includes(filename, file_includes);

        u64 latest_modtime = 0;
        os_get_last_write_time(filename, &latest_modtime);

        for (int i = 0; i < file_includes.count; i++) {
            char *includename = file_includes[i];
            u64 include_modtime = 0;
            os_get_last_write_time(includename, &include_modtime);

            if (include_modtime > latest_modtime) {
                latest_modtime = include_modtime;
            }
        }

        if (latest_modtime > exe_modtime) {
            files_to_compile.add(filename);
        }
    }

    if (!files_to_compile.count) return;
    
    // @TODO: Make a String_Builder
    Array <char> compiler_line;

    {
        char *line = "cl /c /nologo /W3 /diagnostics:column /WL /FC /Oi /EHsc /Zc:strictStrings- /std:c++17 /D_CRT_SECURE_NO_WARNINGS ";
        compiler_line.add(line, get_string_length(line));
    }
    bool debug_symbols = project->debug_symbols;
    if (configuration->debug_symbols_set) debug_symbols = configuration->debug_symbols;
    bool optimize = project->optimize;
    if (configuration->optimize_set) optimize = configuration->optimize;
    bool static_runtime = project->static_runtime;
    if (configuration->static_runtime_set) static_runtime = configuration->static_runtime;

    Runtime_Type runtime_type = project->runtime_type;
    if (static_runtime) {
        compiler_line.add('/');
        compiler_line.add('M');
        compiler_line.add('T');
    } else {
        compiler_line.add('/');
        compiler_line.add('M');
        compiler_line.add('D');
    }

    if (runtime_type == RUNTIME_DEBUG) {
        compiler_line.add('d');
    }

    compiler_line.add(' ');

    if (optimize) {
        char *line = "/O2 /Ob2 ";
        compiler_line.add(line, get_string_length(line));
    } else {
        char *line = "/Od /Ob0 ";
        compiler_line.add(line, get_string_length(line));
    }

    if (debug_symbols) {
        char *line = "/Zi /DEBUG ";
        compiler_line.add(line, get_string_length(line));
    }

    if (pchheader && !pchsource) {
        log_error("pchheader set, but pchsource isn't\n");
        exit(1);
    }

    if (pchsource && !pchheader) {
        log_error("pchsource set, but pchheader isn't\n");
        exit(1);
    }

    if (pchsource && pchheader) {
        char *_pchname = copy_string(pchsource); // @LeakMaybe
        char *slash = strrchr(_pchname, '\\');
        slash += 1;
        char *dot = strrchr(slash, '.');
        if (dot) {
            slash[dot - slash] = 0;
        }
        // @Speed
        char *pchname = new char[get_string_length(slash) + 4];
        memcpy(pchname, slash, get_string_length(slash));
        memcpy(pchname + get_string_length(slash), ".pch", 4);

        Array <char> pch_line;
        
        char *line = sprint("cl /c /Yc\"%s\" /nologo /Fo\"%s\\\\\" /Fd\"%s\\\\\" /W3 /diagnostics:column /WL /FC /Oi /EHsc /Zc:strictStrings- /std:c++17 /D_CRT_SECURE_NO_WARNINGS %s ", pchheader, objdir, outputdir, pchsource);
        pch_line.add(line, get_string_length(line));

        if (static_runtime) {
            pch_line.add('/');
            pch_line.add('M');
            pch_line.add('T');
        } else {
            pch_line.add('/');
            pch_line.add('M');
            pch_line.add('D');
        }

        if (runtime_type == RUNTIME_DEBUG) {
            pch_line.add('d');
        }

        pch_line.add(' ');
        
        if (optimize) {
            line = "/O2 /Ob2 ";
            pch_line.add(line, get_string_length(line));
        } else {
            line = "/Od /Ob0 ";
            pch_line.add(line, get_string_length(line));
        }

        if (debug_symbols) {
            line = "/Zi /DEBUG ";
            pch_line.add(line, get_string_length(line));
        }

        pch_line.add(0);
        system(pch_line.data);
  
        line = sprint("/Yu\"%s\" ", pchheader);
        compiler_line.add(line, get_string_length(line));
    }
    
    {
        char *line = sprint("/Fo\"%s\\\\\" ", objdir);
        compiler_line.add(line, get_string_length(line));
    }

    {
        char *line = sprint("/Fd\"%s\\\\\" ", outputdir); // Make the .pdb file go into the output directory
        compiler_line.add(line, get_string_length(line));
    }

    Array <char *> includedirs;
    for (int i = 0; i < project->includedirs.count; i++) {
        char *dir = project->includedirs[i];
        includedirs.add(dir);
    }
    for (int i = 0; i < configuration->includedirs.count; i++) {
        char *dir = configuration->includedirs[i];
        includedirs.add(dir);
    }
    
    Array <char *> libdirs;
    for (int i = 0; i < project->libdirs.count; i++) {
        char *dir = project->libdirs[i];
        libdirs.add(dir);
    }
    for (int i = 0; i < configuration->libdirs.count; i++) {
        char *dir = configuration->libdirs[i];
        libdirs.add(dir);
    }

    Array <char *> libs;
    for (int i = 0; i < project->libs.count; i++) {
        char *dir = project->libs[i];
        libs.add(dir);
    }
    for (int i = 0; i < configuration->libs.count; i++) {
        char *dir = configuration->libs[i];
        libs.add(dir);
    }

    Array <char *> defines;
    for (int i = 0; i < project->defines.count; i++) {
        char *dir = project->defines[i];
        defines.add(dir);
    }
    for (int i = 0; i < configuration->defines.count; i++) {
        char *dir = configuration->defines[i];
        defines.add(dir);
    }
    
    for (int i = 0; i < includedirs.count; i++) {
        char *dir = includedirs[i];
        replace_forwardslash_with_backslash(dir);
        
        dir = do_macro_substitutions(dir, project, configuration); // @Leak
        
        char *line = sprint("/I %s ", dir);
        compiler_line.add(line, get_string_length(line));
    }
    
    for (int i = 0; i < files_to_compile.count; i++) {
        char *filename = files_to_compile[i];
        compiler_line.add(filename, get_string_length(filename));
        compiler_line.add(' ');
    }

    for (int i = 0; i < defines.count; i++) {
        char *define = defines[i];
        char *line = sprint("/D%s ", define);
        compiler_line.add(line, get_string_length(line));
    }

    compiler_line.add(0);

    Array <char> linker_line;
    {
        char *line = "link /nologo ";
        if (project->type == OUTPUT_STATIC_LIB) {
            line = "lib /nologo ";
        }
        linker_line.add(line, get_string_length(line));
    }

    for (int i = 0; i < libdirs.count; i++) {
        char *dir = libdirs[i];
        replace_forwardslash_with_backslash(dir);

        dir = do_macro_substitutions(dir, project, configuration); // @Leak
        
        char *line = sprint("/LIBPATH:\"%s\" ", dir);
        linker_line.add(line, get_string_length(line));
    }

    for (int i = 0; i < libs.count; i++) {
        char *lib = libs[i];
        replace_forwardslash_with_backslash(lib);
        char *line = sprint("%s ", lib);
        linker_line.add(line, get_string_length(line));
    }

    if (pchsource) {
        project->cfiles.add(pchsource);
    }
    for (int i = 0; i < project->cfiles.count; i++) {
        char *filename = project->cfiles[i];

        char *dir_with_name = temp_copy_strip_extension(filename);
        dir_with_name = replace_backslash_with_forwardslash(dir_with_name);
        char *slash = strrchr(dir_with_name, '/');
        
        Array <char> name;
        if (slash) {
            for (char *at = slash + 1; *at; at++) {
                name.add(*at);
            }
        } else {
            name.add(dir_with_name, get_string_length(dir_with_name));
        }
        name.add(0);
        
        char *line = sprint("%s\\%s.obj ", objdir, name.data);
        linker_line.add(line, get_string_length(line));
    }
    
    if (debug_symbols && project->type != OUTPUT_STATIC_LIB) {
        char *line = "/DEBUG ";
        linker_line.add(line, get_string_length(line));
    }
    
    if (project->type == OUTPUT_CONSOLE_APP) {
        char *line = "/subsystem:console ";
        linker_line.add(line, get_string_length(line));
    } else if (project->type == OUTPUT_WINDOWED_APP) {
        char *line = "/subsystem:windows ";
        linker_line.add(line, get_string_length(line));
    }

    {
        char *extension = "exe";
        if (project->type == OUTPUT_STATIC_LIB) {
            extension = "lib";
        }
        char *line = sprint("/OUT:%s\\%s.%s ", outputdir, outputname, extension);
        defer { delete [] line; };
        linker_line.add(line, get_string_length(line));
    }

    linker_line.add(0);
    
    double rsc_end_time = os_get_time();
    double rsc_time = rsc_end_time - rsc_start_time;

    double cl_start_time = os_get_time();
    log("Compiler line: %s\n", compiler_line.data);
    int result = system(compiler_line.data);
    if (result != 0) {
        os_delete_file(exepath);
        exit(1);
    }
    double cl_end_time = os_get_time();
    double cl_time = cl_end_time - cl_start_time;

    double linker_start_time = os_get_time();
    log("Linker line: %s\n", linker_line.data);
    result = system(linker_line.data);
    if (result != 0) {
        os_delete_file(exepath);
        exit(1);
    }
    double linker_end_time = os_get_time();
    double linker_time = linker_end_time - linker_start_time;

    log("Total time: %.4f\n", rsc_time + cl_time + linker_time);
    log("RSC time: %.4f\n", rsc_time);
    log("MSVC time: %.4f\n", cl_time);
    log("Linker time: %.4f\n", linker_time);
}

int main(int argc, char **argv) {
#ifdef _DEBUG
    system("vars.bat");
#endif
    
    rsc_start_time = os_get_time();
    
    os_init_colors_and_utf8();
    
    init_temporary_storage(Megabytes(1));
    reset_temporary_storage();
    
    Command_Line_Arguments args = parse_command_line_arguments(argc, argv);
    if (!args.rsc_file) {
        log_error("ERROR: No .rsc file provided\n");
        print_usage();
        return 1;
    }
    if (!args.configuration) {
        log_error("ERROR: No configuration provided\n");
        print_usage();
        return 1;
    }

    char *rsc_filepath = args.rsc_file;
    replace_backslash_with_forwardslash(rsc_filepath);

    u64 rsc_modtime = 0;
    os_get_last_write_time(rsc_filepath, &rsc_modtime);
    
    Rsc_Data *data = new Rsc_Data();
    data->should_rebuild_all = args.rebuild_all;
    data->parse_file(rsc_filepath);

    Configuration *current_configuration = NULL;
    for (int i = 0; i < data->configurations.count; i++) {
        Configuration *cfg = data->configurations[i];

        char *cfg_name = temp_copy_string(args.configuration);
        lowercase(cfg_name);
        
        if (strings_match(cfg->lowercased_name, cfg_name)) {
            current_configuration = cfg;
        }
    }
    if (!current_configuration) {
        log_error("ERROR: configuration passed to -configuration: isn't a valid configuration\n");
        exit(1);
    }
    
    for (int i = 0; i < data->projects.count; i++) {
        Rsc_Project *project = data->projects[i];
        execute_msvc_for_project(data, project, current_configuration, rsc_modtime);
    }
    
    return 0;
}
