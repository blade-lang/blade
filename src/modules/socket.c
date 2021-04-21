#include "socket.h"

#include <string.h>


CREATE_MODULE_LOADER(socket) {

  static b_func_reg http_class_functions[] = {
      {NULL, false, NULL},
  };

  static b_field_reg http_class_fields[] = {
      {NULL, false, NULL},
  };

  static b_class_reg classes[] = {
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL, classes};

  return module;
}