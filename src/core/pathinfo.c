#include "pathinfo.h"
#include "common.h"
#include "compat/unistd.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#if defined _WIN32
#include <shlwapi.h>
#include <io.h>
#include <windows.h>

// #define access _access_s
#endif

#ifdef __APPLE__
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h>
#endif

#if defined __linux__ || defined __CYGWIN__ || defined __MINGW32_MAJOR_VERSION
#include <libgen.h>
#include <limits.h>

#if defined(__sun)
#define PROC_SELF_EXE "/proc/self/path/a.out"
#else
#define PROC_SELF_EXE "/proc/self/exe"
#endif

#endif

#if defined _WIN32

#include "win32.h"

char *get_exe_dir() {
    char* exe_path = (char*)malloc(sizeof(char) * MAX_PATH);
    if (exe_path != NULL) {
        int length = (int)GetModuleFileNameA(NULL, exe_path, MAX_PATH);
        if (length > 0) {
            char* path = dirname(exe_path);
            path[(int)strlen(path) - 1] = '\0';
            return path;

        }
        else {
            return NULL;
        }
    }
  return NULL;
}

#endif

#if defined __linux__ ||  defined __CYGWIN__

char *get_exe_path() {
  char raw_path[PATH_MAX];
  realpath(PROC_SELF_EXE, raw_path);
  return raw_path;
}

#endif

#ifdef __APPLE__
char *get_exe_path() {
  char raw_path[PATH_MAX];
  char *real_path = malloc(PATH_MAX * sizeof(char));
  uint32_t raw_size = (uint32_t)sizeof(raw_path);

  if (!_NSGetExecutablePath(raw_path, &raw_size)) {
    realpath(raw_path, real_path);
  }
  return real_path;
}
#endif

#if defined __CYGWIN__ || defined __linux__ || defined __APPLE__

char *get_exe_dir() {
  char *exe_path = get_exe_path();
  char *exe_path_str = malloc((strlen(exe_path) + 1) * sizeof(char));
  strcpy(exe_path_str, exe_path);
  char *exe_dir = dirname(exe_path_str);
  free(exe_path_str);
  return exe_dir;
}
#endif



char* merge_paths(char* a, char* b) {
    char* final_path = "";

    // edge cases
    // 1. a itself is a file
    // 2. b is a bird runtime constant such as <repl>, <script> and <module>

    if (strstr(a, ".") == NULL && strstr(b, "<") == NULL && strlen(a) > 0) {
        final_path = append_strings(final_path, a);
    }
    if (strlen(a) > 0 && b[0] != '.' && strstr(a, ".") == NULL &&
        strstr(b, "<") == NULL) {
        final_path = append_strings(final_path, BIRD_PATH_SEPARATOR);
        final_path = append_strings(final_path, b);
  }
    else {
        final_path = append_strings(final_path, b);
    }
    return final_path;
}

bool file_exists(char *filepath) { return access(filepath, F_OK) == 0; }

#ifndef _WIN32
char* get_calling_dir() { return getenv("PWD"); }

char* get_filename(char* filepath) {
    int start = 0, length = strlen(filepath);
    for (int i = 0; i < length; i++) {
        if (filepath[i] == BIRD_PATH_SEPARATOR[0])
            start = i;
    }
    length = length - start;
    char* string = malloc(sizeof(char));
    strncat(string, filepath + start, length);
    return string;
}
#else
char* get_calling_dir() { return _fullpath(NULL, "", 0); }

char* get_filename(char* filepath) {
    char* file = (char*)malloc(sizeof(char));
    char* ext = (char*)malloc(sizeof(char));
    _splitpath((const char*)filepath, NULL, NULL, file, ext);
    return merge_paths(file, ext);
}
#endif // !_WIN32

char *get_bird_filename(char *filename) {
  return merge_paths(filename, BIRD_EXTENSION);
}

char *resolve_import_path(char *module_name, const char *current_file) {
  char *bird_file_name = get_bird_filename(module_name);

  // check relative to the current file...
  char *file_directory = dirname((char *)current_file);
  char *relative_file = merge_paths(file_directory, bird_file_name);

  if (file_exists(relative_file))
    return relative_file;

  // check in bird's default location
  char *bird_directory = merge_paths(get_exe_dir(), LIBRARY_DIRECTORY);
  char *library_file = merge_paths(bird_directory, bird_file_name);

  if (file_exists(library_file))
    return library_file;

  return NULL;
}

#include <stdio.h>
bool is_core_library_file(char *filepath, char *file_name) {
  char *bird_file_name = get_bird_filename(file_name);
  char *bird_directory = merge_paths(get_exe_dir(), LIBRARY_DIRECTORY);
  char *library_file = merge_paths(bird_directory, bird_file_name);

  if (file_exists(library_file)) {
    return memcmp(library_file, filepath, (int)strlen(filepath)) == 0;
  }
  return false;
}