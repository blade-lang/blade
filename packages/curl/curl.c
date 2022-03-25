#include <blade.h>
#include <curl/curl.h>

void b__curl_module_preloader(b_vm *vm) {
  curl_global_init(CURL_GLOBAL_ALL);
}

void b__curl_module_unloader(b_vm *vm) {
  curl_global_cleanup();
}

DECLARE_MODULE_METHOD(curl__init) {
  RETURN;
}

CREATE_MODULE_LOADER(curl) {
  static b_func_reg module_functions[] = {
      {"init",   true,  GET_MODULE_METHOD(curl__init)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_curl",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = &b__curl_module_preloader,
      .unloader = &b__curl_module_unloader
  };

  return &module;
}
