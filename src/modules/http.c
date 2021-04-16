#include "http.h"


CREATE_MODULE_LOADER(http) {
  static b_func_reg http_class_functions[] = {
      {NULL,    false, NULL},
  };

  static b_field_reg http_class_fields[] = {
      {NULL,       false, NULL},
  };

  static b_class_reg classes[] = {
      {"Http", http_class_fields, http_class_functions},
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL, classes};

  return module;
}