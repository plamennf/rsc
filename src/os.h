#pragma once

#include "general.h"

double os_get_time();

char *os_read_entire_file(char *filepath, s64 *out_length = nullptr);
bool os_file_exists(char *filepath);
void os_get_last_write_time(char *file_path, u64 *out_time);
bool os_delete_file(char *file);

void os_setcwd(char *dir);

void os_init_colors_and_utf8();

bool os_directory_exists(char *dir);
bool os_make_directory_if_not_exist(char *dir);
