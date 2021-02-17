#include "base64.h"

#include <math.h>

DECLARE_MODULE_METHOD(math__sin) {
  ENFORCE_ARG_COUNT(sin, 1);
  ENFORCE_ARG_TYPE(sin, 0, IS_NUMBER);
  RETURN_NUMBER(sin(AS_NUMBER(args[0])));
}

static b_func_reg class_functions[] = {
    {"sin", true, GET_MODULE_METHOD(math__sin)},
    {NULL, false, NULL},
};

static b_class_reg klasses[] = {
    {"Math", class_functions},
    {NULL, NULL},
};

static b_module_reg module = {NULL, klasses};

CREATE_MODULE_LOADER(math) { return module; }