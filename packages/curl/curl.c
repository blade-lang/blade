#include <blade.h>
#include "curl_setup.h"
#include <curl/curl.h>

DECLARE_MODULE_METHOD(curl__init) {
  RETURN;
}

CREATE_MODULE_LOADER(json) {
  static b_func_reg module_functions[] = {
      {"init",   true,  GET_MODULE_METHOD(curl__init)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_curl",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}
