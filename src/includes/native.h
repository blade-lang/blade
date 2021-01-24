#ifndef bird_native_h
#define bird_native_h

#include "object.h"
#include "util.h"
#include "value.h"

#define DECLARE_NATIVE(name)                                                   \
  b_value native_fn_##name(b_vm *vm, int arg_count, b_value *args)

#define DECLARE_METHOD(name)                                                   \
  b_value native_method_##name(b_vm *vm, int arg_count, b_value *args)

#define GET_NATIVE(name) native_fn_##name
#define GET_METHOD(name) native_method_##name

#define DEFINE_NATIVE(name) define_native(vm, #name, GET_NATIVE(name))

#define DEFINE_METHOD(table, name)                                             \
  define_native_method(vm, &vm->methods_##table, #name,                        \
                       native_method_##table##name)

#define METHOD_OBJECT peek(vm, arg_count)

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

#define ENFORCE_MIN_ARG(name, d)                                               \
  if (arg_count < d) {                                                         \
    _runtime_error(vm, #name "() expects minimum of %d arguments, %d given",   \
                   d, arg_count);                                              \
    return EMPTY_VAL;                                                          \
  }

#define ENFORCE_MAX_ARG(name, d)                                               \
  if (arg_count < d) {                                                         \
    _runtime_error(vm, #name "() expects maximum of %d arguments, %d given",   \
                   d, arg_count);                                              \
    return EMPTY_VAL;                                                          \
  }

#define ENFORCE_ARG_RANGE(name, low, up)                                       \
  if (arg_count < low || arg_count > up) {                                     \
    _runtime_error(vm,                                                         \
                   #name "() expects between %d and %d arguments, %d given",   \
                   low, up, arg_count);                                        \
    return EMPTY_VAL;                                                          \
  }

#define ENFORCE_ARG_TYPE(name, i, type)                                        \
  if (!type(args[i])) {                                                        \
    _runtime_error(                                                            \
        vm, #name "() expects argument %d as " NORMALIZE(type) ", %s given",   \
        i + 1, value_type(args[i]));                                           \
    return EMPTY_VAL;                                                          \
  }

#define METHOD_OVERRIDE(override, i)                                           \
  do {                                                                         \
    if (IS_INSTANCE(args[0])) {                                                \
      b_obj_instance *instance = AS_INSTANCE(args[0]);                         \
      if (invoke_from_class(vm, instance->klass,                               \
                            copy_string(vm, #override, i), 0)) {               \
        RETURN;                                                                \
      }                                                                        \
    }                                                                          \
  } while (0);

#define RETURN return NIL_VAL
#define RETURN_ERROR return EMPTY_VAL
#define RETURN_BOOL(v) return BOOL_VAL(v)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_NUMBER(v) return NUMBER_VAL(v)
#define RETURN_OBJ(v) return OBJ_VAL(v)
#define RETURN_STRING(v) return OBJ_VAL(copy_string(vm, v, (int)strlen(v)))
#define RETURN_VALUE(v) return v

DECLARE_NATIVE(time);
DECLARE_NATIVE(microtime);
DECLARE_NATIVE(id);
DECLARE_NATIVE(hash);
DECLARE_NATIVE(hasprop);
DECLARE_NATIVE(getprop);
DECLARE_NATIVE(setprop);
DECLARE_NATIVE(delprop);
DECLARE_NATIVE(max);
DECLARE_NATIVE(min);
DECLARE_NATIVE(sum);
DECLARE_NATIVE(abs);
DECLARE_NATIVE(int);
DECLARE_NATIVE(hex);

DECLARE_NATIVE(oct);
DECLARE_NATIVE(bin);
DECLARE_NATIVE(ord);
DECLARE_NATIVE(chr);
DECLARE_NATIVE(to_dict);
DECLARE_NATIVE(to_list);
DECLARE_NATIVE(to_int);
DECLARE_NATIVE(to_number);
DECLARE_NATIVE(to_string);
DECLARE_NATIVE(to_bool);

DECLARE_NATIVE(rand);

DECLARE_NATIVE(print);

#endif