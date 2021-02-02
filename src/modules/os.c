#include "modules/os.h"
#include "compat/unistd.h"
#include <sys/utsname.h>

#include <stdio.h>

DECLARE_MODULE_METHOD(os, exec) {
  ENFORCE_ARG_COUNT(exec, 1);
  ENFORCE_ARG_TYPE(exec, 0, IS_STRING);
  b_obj_string *string = AS_STRING(args[0]);
  if (string->length == 0) {
    RETURN;
  }

  FILE *fd = popen(string->chars, "r");
  if (!fd)
    RETURN;

  char buffer[256];
  size_t nread;
  size_t output_size = 256;
  size_t length = 0;
  char *output = malloc(output_size);

  while ((nread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
    if (length + nread >= output_size) {
      output_size *= 2;
      output = realloc(output, output_size);
    }
    strncat(output + length, buffer, nread);
    length += nread;
  }

  pclose(fd);
  RETURN_STRING(output);
}

DECLARE_MODULE_METHOD(os, info) {
  ENFORCE_ARG_COUNT(info, 0);
  struct utsname os;
  if (uname(&os) != 0) {
    RETURN_ERROR("could not access os information");
  }

  b_obj_dict *dict = new_dict(vm);
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "sysname", 7)),
                 OBJ_VAL(copy_string(vm, os.sysname, strlen(os.sysname))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "nodename", 8)),
                 OBJ_VAL(copy_string(vm, os.nodename, strlen(os.nodename))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "version", 7)),
                 OBJ_VAL(copy_string(vm, os.version, strlen(os.version))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "release", 7)),
                 OBJ_VAL(copy_string(vm, os.release, strlen(os.release))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "machine", 7)),
                 OBJ_VAL(copy_string(vm, os.machine, strlen(os.machine))));

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(os, sleep) {
  ENFORCE_ARG_COUNT(sleep, 1);
  ENFORCE_ARG_TYPE(sleep, 0, IS_NUMBER);
  sleep((int)AS_NUMBER(args[0]));
  RETURN;
}

CREATE_MODULE_LOADER(os) {

  b_class_reg *klass = new_class_reg("Os");
  add_class_method(klass, "info", GET_MODULE_METHOD(os, info));
  add_class_method(klass, "exec", GET_MODULE_METHOD(os, exec));
  add_class_method(klass, "sleep", GET_MODULE_METHOD(os, sleep));

  b_module *module = new_module();
  add_module_class(module, klass);

  return module;
}