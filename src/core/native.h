#ifndef bird_native_h
#define bird_native_h

#include "config.h"

#include "memory.h"
#include "object.h"
#include "util.h"
#include "value.h"

#include "pcre2.h"

#define DECLARE_NATIVE(name)                                                   \
  b_value native_fn_##name(b_vm *vm, int arg_count, b_value *args)

#define DECLARE_METHOD(name)                                                   \
  b_value native_method_##name(b_vm *vm, int arg_count, b_value *args)

#define DECLARE_MODULE_METHOD(name)                                            \
  b_value native_module_##name(b_vm *vm, int arg_count, b_value *args)

#define GET_NATIVE(name) native_fn_##name
#define GET_METHOD(name) native_method_##name
#define GET_MODULE_METHOD(name) native_module_##name

#define DEFINE_NATIVE(name) define_native(vm, #name, GET_NATIVE(name))

#define DEFINE_METHOD(table, name)                                             \
  define_native_method(vm, &vm->methods_##table, #name,                        \
                       native_method_##table##name)

#define METHOD_OBJECT peek(vm, arg_count)

#define NORMALIZE_IS_BOOL "bool"
#define NORMALIZE_IS_BYTES "bytes"
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

#define RETURN return NIL_VAL
#define RETURN_ERROR(...)                                                      \
  {                                                                            \
    _runtime_error(vm, ##__VA_ARGS__);                                         \
    return UNDEFINED_VAL;                                                      \
  }
#define RETURN_EMPTY return EMPTY_VAL
#define RETURN_BOOL(v) return BOOL_VAL(v)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_NUMBER(v) return NUMBER_VAL(v)
#define RETURN_OBJ(v) return OBJ_VAL(v)
#define RETURN_STRING(v) return OBJ_VAL(copy_string(vm, v, (int)strlen(v)))
#define RETURN_LSTRING(v, l) return OBJ_VAL(copy_string(vm, v, l))
#define RETURN_TSTRING(v, l) return OBJ_VAL(take_string(vm, v, l))
#define RETURN_VALUE(v) return v

#define ENFORCE_ARG_COUNT(name, d)                                             \
  if (arg_count != d) {                                                        \
    RETURN_ERROR(#name "() expects %d arguments, %d given", d, arg_count);     \
  }

#define ENFORCE_MIN_ARG(name, d)                                               \
  if (arg_count < d) {                                                         \
    RETURN_ERROR(#name "() expects minimum of %d arguments, %d given", d,      \
                 arg_count);                                                   \
  }

#define ENFORCE_MAX_ARG(name, d)                                               \
  if (arg_count < d) {                                                         \
    RETURN_ERROR(#name "() expects maximum of %d arguments, %d given", d,      \
                 arg_count);                                                   \
  }

#define ENFORCE_ARG_RANGE(name, low, up)                                       \
  if (arg_count < low || arg_count > up) {                                     \
    RETURN_ERROR(#name "() expects between %d and %d arguments, %d given",     \
                 low, up, arg_count);                                          \
  }

#define ENFORCE_ARG_TYPE(name, i, type)                                        \
  if (!type(args[i])) {                                                        \
    RETURN_ERROR(#name                                                         \
                 "() expects argument %d as " NORMALIZE(type) ", %s given",    \
                 i + 1, value_type(args[i]));                                  \
  }

#define EXCLUDE_ARG_TYPE(method_name, arg_type, index)                         \
  if (arg_type(args[index])) {                                                 \
    RETURN_ERROR("invalid type %s() as argument %d in %s()",                   \
                 value_type(args[index]), index + 1, #method_name);            \
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

#define REGEX_COMPILATION_ERROR(re, error_number, error_offset)                \
  if (re == NULL) {                                                            \
    PCRE2_UCHAR8 buffer[256];                                                  \
    pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));           \
    RETURN_ERROR("regular expression compilation failed at offset %d: %s",     \
                 (int)error_offset, buffer);                                   \
  }

#define REGEX_ASSERTION_ERROR(re, match_data, ovector)                         \
  if (ovector[0] > ovector[1]) {                                               \
    runtime_error(                                                             \
        "match aborted: regular expression used \\K in an assertion %.*s to "  \
        "set match start after its end.",                                      \
        (int)(ovector[0] - ovector[1]), (char *)(subject + ovector[1]));       \
    pcre2_match_data_free(match_data);                                         \
    pcre2_code_free(re);                                                       \
    return EMPTY_VAL;                                                          \
  }

#define REGEX_RC_ERROR() RETURN_ERROR("regular expression error %d", rc);

#define GET_REGEX_COMPILE_OPTIONS(name, string, regex_show_error)              \
  uint32_t compile_options = is_regex(string);                                 \
  if (regex_show_error && (int)compile_options == -1) {                        \
    RETURN_ERROR("invalid regular expression passed to " #name "()");          \
  } else if (regex_show_error && (int)compile_options < -1) {                  \
    RETURN_ERROR("invalid regular expression delimiter or character %c "       \
                 "supplied to " #name "()",                                    \
                 (char)abs((int)compile_options));                             \
  }

extern int is_regex(b_obj_string *string);
extern char *remove_regex_delimiter(b_vm *vm, b_obj_string *string);
extern void write_list(b_vm *vm, b_obj_list *list, b_value value);
extern b_obj_list *copy_list(b_vm *vm, b_obj_list *list, int start, int length);

DECLARE_NATIVE(time);
DECLARE_NATIVE(microtime);

DECLARE_NATIVE(hasprop);
DECLARE_NATIVE(getprop);
DECLARE_NATIVE(setprop);
DECLARE_NATIVE(delprop);

DECLARE_NATIVE(id);
DECLARE_NATIVE(hash);

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

DECLARE_NATIVE(type);

DECLARE_NATIVE(is_callable);
DECLARE_NATIVE(is_bool);
DECLARE_NATIVE(is_number);
DECLARE_NATIVE(is_int);
DECLARE_NATIVE(is_string);
DECLARE_NATIVE(is_list);
DECLARE_NATIVE(is_dict);
DECLARE_NATIVE(is_object);
DECLARE_NATIVE(is_function);
DECLARE_NATIVE(is_class);
DECLARE_NATIVE(is_instance);

DECLARE_NATIVE(print);

#endif