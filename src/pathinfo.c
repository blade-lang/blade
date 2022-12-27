#if defined(_MSC_VER)
#pragma warning(disable : 5105)
#endif

#include "pathinfo.h"
#include "common.h"

#ifdef _WIN32

char *get_exe_dir() {
  char *exe_path = (char *) malloc(sizeof(char) * MAX_PATH);
  if (exe_path != NULL) {
    int length = (int) GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (length > 0) {
      char *path = dirname(exe_path);
      return path;
    } else {
      return NULL;
    }
  }
  return NULL;
}

#elif defined(__linux__) || defined(__CYGWIN__)

char *get_exe_path() {
  char raw_path[PATH_MAX];
  ssize_t read_length;
  if ((read_length = readlink(PROC_SELF_EXE, raw_path, sizeof(raw_path))) >
          -1 &&
      read_length < PATH_MAX) {
    return strdup(raw_path);
  }
  return "";
}

#elif defined(__APPLE__)

char *get_exe_path() {
  char raw_path[PATH_MAX];
  char *real_path = (char *) malloc(PATH_MAX * sizeof(char));
  uint32_t raw_size = (uint32_t) sizeof(raw_path);

  if (!_NSGetExecutablePath(raw_path, &raw_size)) {
    realpath(raw_path, real_path);
  }
  return real_path;
}

#endif

#if defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)

char *get_exe_dir() { return dirname(get_exe_path()); }

#endif

char *merge_paths(char *a, char *b) {
  char *final_path = (char *) calloc(1, sizeof(char));

  // by checking b first, we guarantee that b is neither NULL nor
  // empty by the time we are checking a so that we can return a
  // duplicate of b
  int len_b = (int)strlen(b);
  if(b == NULL || len_b == 0) {
    free(final_path);
    return strdup(a); // just in case a is const char*
  } if(a == NULL || strlen(a) == 0) {
    free(final_path);
    return strdup(b); // just in case b is const char*
  }

  final_path = append_strings(final_path, a);

  if(!(len_b == 2 && b[0] == '.' && b[1] == 'b')) {
    final_path = append_strings(final_path, BLADE_PATH_SEPARATOR);
  }
  final_path = append_strings(final_path, b);
  return final_path;
}

bool file_exists(char *filepath) { return access(filepath, F_OK) == 0; }

char *get_blade_filename(char *filename) {
  return merge_paths(filename, BLADE_EXTENSION);
}

char *resolve_import_path(char *module_name, const char *current_file, char *root_file, bool is_relative) {
  printf("Importing %s at root %s\n", module_name, root_file);
  char *blade_file_name = get_blade_filename(module_name);

  // check relative to the current file...
  char *file_directory = dirname((char *) strdup(current_file));

  // fixing last path / if exists (looking at windows)...
  int file_directory_length = (int) strlen(file_directory);
  if (file_directory[file_directory_length - 1] == '\\') {
    file_directory[file_directory_length - 1] = '\0';
  }

  // search system library if we are not looking for a relative module.
  if (!is_relative) {

    // firstly, search the local vendor directory for a matching module
    char *root_dir = root_file == NULL ? getcwd(NULL, 0) : dirname(strdup(root_file));
    // fixing last path / if exists (looking at windows)...
    int root_dir_length = (int) strlen(root_dir);
    if (root_dir[root_dir_length - 1] == '\\') {
      root_dir[root_dir_length - 1] = '\0';
    }

    char *vendor_file = merge_paths(merge_paths(root_dir,
          LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), blade_file_name);
    if (file_exists(vendor_file)) {
      // stop a core library from importing itself
      char *path1 = realpath(vendor_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // or a matching package
    char *vendor_index_file = merge_paths(merge_paths(merge_paths(root_dir,
          LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), module_name), LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
    if (file_exists(vendor_index_file)) {
      // stop a core library from importing itself
      char *path1 = realpath(vendor_index_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // then, check in blade's default locations
    char *exe_dir = get_exe_dir();
    char *blade_directory = merge_paths(exe_dir, LIBRARY_DIRECTORY);

    // check blade libs directory for a matching module...
    char *library_file = merge_paths(blade_directory, blade_file_name);
    if (file_exists(library_file)) {
      // stop a core library from importing itself
      char *path1 = realpath(library_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // check blade libs directory for a matching package...
    char *library_index_file = merge_paths(merge_paths(blade_directory, module_name),
           get_blade_filename(LIBRARY_DIRECTORY_INDEX));
    if (file_exists(library_index_file)) {
      char *path1 = realpath(library_index_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // check blade vendor directory installed module...
    char *blade_package_directory = merge_paths(exe_dir, PACKAGES_DIRECTORY);
    char *package_file = merge_paths(blade_package_directory, blade_file_name);
    if (file_exists(package_file)) {
      char *path1 = realpath(package_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // check blade vendor directory installed package...
    char *package_index_file = merge_paths(merge_paths(blade_package_directory, module_name),
                                           LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
    if (file_exists(package_index_file)) {
      char *path1 = realpath(package_index_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }
  } else {

    // otherwise, search the relative path for a matching module
    char *relative_file = merge_paths(file_directory, blade_file_name);
    if (file_exists(relative_file)) {
      // stop a user module from importing itself
      char *path1 = realpath(relative_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }

    // or a matching package
    char *relative_index_file = merge_paths(merge_paths(file_directory, module_name),
          get_blade_filename(LIBRARY_DIRECTORY_INDEX));
    if (file_exists(relative_index_file)) {
      char *path1 = realpath(relative_index_file, NULL);
      char *path2 = realpath(current_file, NULL);

      if (path1 != NULL) {
        if (path2 == NULL || memcmp(path1, path2, (int) strlen(path2)) != 0)
          return path1;
      }
    }
  }

  return NULL;
}

char *get_real_file_name(char *path) { return basename(path); }
