#ifndef bird_module_h
#define bird_module_h

#include "native.h"
#include "object.h"
#include "value.h"

typedef struct {
  const char *name;
  b_native_fn function;
} b_func_reg;

typedef struct {
  const char *name;
  int function_count;
  const char **names;
  b_native_fn *functions;
} b_class_reg;

typedef struct {
  int function_count;
  int class_count;
  b_func_reg *functions;
  b_class_reg **klasses;
} b_module;

static b_module *new_module() {
  b_module *reg = calloc(1, sizeof(b_module));
  reg->function_count = 0;
  reg->class_count = 0;
  reg->functions = NULL;
  reg->klasses = NULL;
  return reg;
}

static b_class_reg *new_class_reg(const char *name) {
  b_class_reg *reg = calloc(1, sizeof(b_class_reg));
  reg->function_count = 0;
  reg->functions = NULL;
  reg->names = NULL;
  reg->name = name;
  return reg;
}

static void add_class_method(b_class_reg *klass, const char *name,
                             b_native_fn fn) {
  klass->functions =
      realloc(klass->functions, sizeof(b_native_fn *) * klass->function_count);
  klass->names =
      realloc(klass->names, sizeof(b_native_fn *) * (klass->function_count));
  klass->functions[klass->function_count] = fn;
  klass->names[klass->function_count] = name;
  klass->function_count++;
}

static void add_module_method(b_module *module, const char *name,
                              b_native_fn fn) {
  b_func_reg reg;
  reg.name = name;
  reg.function = fn;

  module->functions = realloc(module->functions,
                              sizeof(b_func_reg) * (module->function_count++));
  module->functions[module->function_count - 1] = reg;
}

static void add_module_class(b_module *module, b_class_reg *klass) {
  module->klasses =
      realloc(module->klasses, sizeof(b_class_reg *) * (module->class_count++));
  module->klasses[module->class_count - 1] = klass;
}

#define CREATE_MODULE_LOADER(module)                                           \
  b_module *bird_module_loader_##module(b_vm *vm)
#define GET_MODULE_LOADER(module) &bird_module_loader_##module

#define REGISTER_FUNCTION(module, name)                                        \
  { #name, &GET_MODULE_METHOD(module, name) }

void bind_native_modules(b_vm *vm, b_obj_string *module_name,
                         const char *module_path);

#endif