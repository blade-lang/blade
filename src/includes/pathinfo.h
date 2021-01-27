#ifndef bird_pathinfo_h
#define bird_pathinfo_h

#include "common.h"

#if defined(__linux__) || defined(__APPLE__)
#define BIRD_PATH_SEPARATOR "/"
#elif defined(_Win32)
#define BIRD_PATH_SEPARATOR "\\"
#else
#define BIRD_PATH_SEPARATOR "/"
#endif

char *get_exe_path();
char *get_exe_dir();
char *get_calling_dir();
char *merge_paths(char *a, char *b, int length);
bool file_exists(char *filepath);
char *get_birdy_filename(char *filename);
char *get_filename(char *filepath);

#endif