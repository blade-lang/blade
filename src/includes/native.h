#ifndef bird_native_h
#define bird_native_h

#include "value.h"

#define DECLARE_NATIVE(name)                                                   \
  b_value native_fn_##name(b_vm *vm, int arg_count, b_value *args)

#define GET_NATIVE(name) native_fn_##name

#define DEFINE_NATIVE(name) define_native(vm, #name, GET_NATIVE(name))

#define NORMALIZE_IS_BOOL "bool"
#define NORMALIZE_IS_NUMBER "number"
#define NORMALIZE_IS_CHAR "char"
#define NORMALIZE_IS_STRING "string"
#define NORMALIZE_IS_CLOSURE "function"
#define NORMALIZE_IS_INSTANCE "instance"
#define NORMALIZE_IS_CLASS "class"
#define NORMALIZE_IS_LIST "list"
#define NORMALIZE_IS_DICT "dict"
#define NORMALIZE_IS_OBJ "object"
#define NORMALIZE_IS_FILE "file"

#define NORMALIZE(token) NORMALIZE_##token

#define ENFORCE_ARG_COUNT(name, d)                                             \
  if (arg_count != d) {                                                        \
    _runtime_error(vm, #name "() expects %d arguments, %d given", d,           \
                   arg_count);                                                 \
    return EMPTY_VAL;                                                          \
  }

#define ENFORCE_ARG_TYPE(name, i, type)                                        \
  if (!type(args[i])) {                                                        \
    _runtime_error(                                                            \
        vm, #name "() expects argument %d as " NORMALIZE(type) ", %s given",   \
        i + 1, value_type(args[i]));                                           \
    return EMPTY_VAL;                                                          \
  }

#define RETURN return NIL_VAL
#define RETURN_BOOL(v) return BOOL_VAL(v)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_NUMBER(v) return NUMBER_VAL(v)

DECLARE_NATIVE(time);
DECLARE_NATIVE(microtime);
DECLARE_NATIVE(id);
DECLARE_NATIVE(hasprop);
DECLARE_NATIVE(getprop);
DECLARE_NATIVE(setprop);
DECLARE_NATIVE(delprop);
DECLARE_NATIVE(max);
DECLARE_NATIVE(min);

#endif