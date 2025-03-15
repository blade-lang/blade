
#include <stddef.h>
#include <stdio.h>
#include "pathinfo.h"
#include "util.h"

int main(int argc, char *argv[]) {

#if defined(_WIN32)
  const char *blade_exe_path  = "runtime\\blade.exe";
  const char *application_path = "app";
#else
  const char *blade_exe_path  = "runtime/blade";
  const char *application_path = "app";
#endif

  char *root_dir = NULL;
  if(file_exists("./macos")) {
    blade_exe_path = "Resources/runtime/blade";
    application_path = "Resources/app";
    root_dir = dirname(get_exe_dir());
  } else {
    root_dir = get_exe_dir();
  }

  // navigate to root directory
  chdir(root_dir);

  /// PREPARATIONS
  char *exe_path = merge_paths(root_dir, (char *)blade_exe_path);
  char *app_root = merge_paths(root_dir, (char *)application_path);

  // 1. Create arguments
  char *cmd = calloc(1, sizeof(char));

  // add exe_path to arguments
  cmd = append_strings_n(cmd, "\"", 1);
  cmd = append_strings(cmd, exe_path);
  cmd = append_strings_n(cmd, "\" ", 2);

  // add app_root to arguments
  cmd = append_strings_n(cmd, "\"", 1);
  cmd = append_strings(cmd, app_root);
  cmd = append_strings_n(cmd, "\"", 1);

  if(argc > 1) {
    for(int i = 1; i < argc; i++) {
      cmd = append_strings_n(cmd, " ", 1);
      cmd = append_strings(cmd, argv[i]);
    }
  }

  /// RUN
  int return_value = system(cmd);

  /// CLEANUP
  free(app_root);
  free(exe_path);
  free(root_dir);

  getchar();
  return return_value;
}
