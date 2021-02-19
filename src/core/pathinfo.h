#ifndef bird_pathinfo_h
#define bird_pathinfo_h

#include "common.h"

#if defined __linux__ || defined __APPLE__

#define BIRD_PATH_SEPARATOR "/"

#elif defined(_WIN32)

#define BIRD_PATH_SEPARATOR "\\"
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)

#else

#define BIRD_PATH_SEPARATOR "/"

#endif

char *get_exe_path();
char *get_exe_dir();
char *get_calling_dir();
char *merge_paths(char *a, char *b);
bool file_exists(char *filepath);
char *get_bird_filename(char *filename);
char *get_filename(char *filepath);
char *resolve_import_path(char *module_name, const char *current_file);
bool is_core_library_file(char *filepath, char *file_name);

#endif