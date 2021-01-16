#ifndef bird_native_h
#define bird_native_h

#include "value.h"

#define DECLARE_NATIVE(name)                                                   \
  b_value native_fn_##name(b_vm *vm, int arg_count, b_value *args)

#define GET_NATIVE(name) native_fn_##name

#define DEFINE_NATIVE(name) define_native(vm, #name, GET_NATIVE(name))

#define RETURN_NUMBER(v) return NUMBER_VAL(v)

DECLARE_NATIVE(time);
DECLARE_NATIVE(microtime);
DECLARE_NATIVE(id);

#endif