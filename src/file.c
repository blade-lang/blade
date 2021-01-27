#include "builtin/file.h"

DECLARE_NATIVE(file) {
  ENFORCE_ARG_RANGE(file, 1, 2);
  ENFORCE_ARG_TYPE(file, 0, IS_STRING);
  b_obj_string *path = AS_STRING(args[0]);

  if (path->length == 0) {
    RETURN_ERROR("file path cannot be empty");
  }

  b_obj_string *mode;

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(file, 1, IS_STRING);
    mode = AS_STRING(args[1]);
  } else {
    mode = copy_string(vm, "r", 1);
  }

  FILE *fp = fopen(path->chars, mode->chars);
  b_obj_file *file = new_file(vm, fp, path, mode);
  RETURN_OBJ(file);
}