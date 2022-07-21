#include <blade.h>
#include <ffi.h>

DECLARE_MODULE_METHOD(clib__init) {
  RETURN;
}

CREATE_MODULE_LOADER(json) {
  static b_func_reg module_functions[] = {
      {"_init",   true,  GET_MODULE_METHOD(clib__init)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_clib",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}