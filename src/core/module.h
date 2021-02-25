#ifndef bird_module_h
#define bird_module_h

#include "native.h"
#include "object.h"
#include "value.h"

typedef struct {
  const char *name;
  bool is_static;
  b_native_fn function;
} b_func_reg;

typedef struct {
  const char *name;
  bool is_static;
  b_value value;
} b_field_reg;

typedef struct {
  const char *name;
  b_field_reg *fields;
  b_func_reg *functions;
} b_class_reg;

typedef struct {
  b_func_reg *functions;
  b_class_reg *klasses;
} b_module_reg;

#define CREATE_MODULE_LOADER(module)                                           \
  b_module_reg bird_module_loader_##module(b_vm *vm)
#define GET_MODULE_LOADER(module) &bird_module_loader_##module

#define REGISTER_FUNCTION(name)                                                \
  { #name, &GET_MODULE_METHOD(name) }

void bind_native_modules(b_vm *vm, b_obj_string *module_name,
                         const char *module_path);

#endif