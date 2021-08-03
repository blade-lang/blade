#ifndef BLADE_MODULE_H
#define BLADE_MODULE_H

#include "native.h"
#include "object.h"
#include "value.h"

typedef struct {
  const char *name;
  bool is_static;
  b_native_fn function;
} b_func_reg;

typedef b_value (*b_class_field)(b_vm *);

typedef void (*b_module_loader)(b_vm *);

typedef struct {
  const char *name;
  bool is_static;
  b_class_field field_value;
} b_field_reg;

typedef struct {
  const char *name;
  b_field_reg *fields;
  b_func_reg *functions;
} b_class_reg;

typedef struct {
  const char *name;
  b_field_reg *fields;
  b_func_reg *functions;
  b_class_reg *classes;
  b_module_loader preloader;
  b_module_loader unloader;
} b_module_reg;

typedef b_module_reg* (*b_module_init)(b_vm *);

#define CREATE_MODULE_LOADER(module)                                           \
  b_module_reg* blade_module_loader_##module(b_vm *vm)
#define GET_MODULE_LOADER(module) &blade_module_loader_##module

void bind_native_modules(b_vm *vm);

#endif