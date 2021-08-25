#ifndef BLADE_NATIVE_H
#define BLADE_NATIVE_H

#include "config.h"

#include "memory.h"
#include "object.h"
#include "util.h"
#include "value.h"

#include "pcre2.h"

#define N__(x, y) 200##y
#define N___(x, y) N__(x, y)
#define NEW_OBJ_TYPE N___(__LINE__, __COUNTER__)

#define DECLARE_NATIVE(name)                                                   \
  bool native_fn_##name(b_vm *vm, int arg_count, b_value *args)

#define DECLARE_METHOD(name)                                                   \
  bool native_method_##name(b_vm *vm, int arg_count, b_value *args)

#define DECLARE_MODULE_METHOD(name)                                            \
  bool native_module_##name(b_vm *vm, int arg_count, b_value *args)

#define GET_NATIVE(name) native_fn_##name
#define GET_METHOD(name) native_method_##name
#define GET_MODULE_METHOD(name) native_module_##name

#define DEFINE_NATIVE(name) define_native(vm, #name, GET_NATIVE(name))

#define DEFINE_METHOD(table, name)                                             \
  define_native_method(vm, &vm->methods_##table, #name,                        \
                       native_method_##table##name)

// NOTE: METHOD_OBJECT must always be retrieved
// before any call to create an object in a native function.
// failure to do so will lead to the first object created
// within the function to appear as METHOD_OBJECT
#define METHOD_OBJECT args[-1]

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

#define RETURN { args[-1] = NIL_VAL; return true; }
#define RETURN_EMPTY { args[-1] = NIL_VAL; return false; }
#define RETURN_ERROR(...)                                                      \
  {                                                                            \
    pop_n(vm, arg_count); \
    throw_exception(vm, ##__VA_ARGS__);                                        \
    args[-1] = FALSE_VAL; \
    return false;                                                          \
  }
#define RETURN_BOOL(v) { args[-1] = BOOL_VAL(v); return true; }
#define RETURN_TRUE { args[-1] = BOOL_VAL(true); return true; }
#define RETURN_FALSE { args[-1] = BOOL_VAL(false); return true; }
#define RETURN_NUMBER(v) { args[-1] = NUMBER_VAL(v); return true; }
#define RETURN_OBJ(v) { args[-1] = OBJ_VAL(v); return true; }
#define RETURN_STRING(v) { args[-1] = OBJ_VAL(copy_string(vm, v, (int)strlen(v))); return true; }
#define RETURN_L_STRING(v, l) { args[-1] = OBJ_VAL(copy_string(vm, v, l)); return true; }
#define RETURN_T_STRING(v, l) { args[-1] = OBJ_VAL(take_string(vm, v, l)); return true; }
#define RETURN_TT_STRING(v) { args[-1] = OBJ_VAL(take_string(vm, v, (int)strlen(v))); return true; }
#define RETURN_VALUE(v) { args[-1] = v; return true; }

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
  if (arg_count < (low) || arg_count > (up)) {                                     \
    RETURN_ERROR(#name "() expects between %d and %d arguments, %d given",     \
                 low, up, arg_count);                                          \
  }

#define ENFORCE_ARG_TYPE(name, i, type)                                        \
  if (!type(args[i])) {                                                        \
    RETURN_ERROR(#name                                                         \
                 "() expects argument %d as " NORMALIZE(type) ", %s given",    \
                 (i) + 1, value_type(args[i]));                                  \
  }

#define ENFORCE_CONSTRUCTOR_ARG_TYPE(name, i, type)                            \
  if (!type(args[i])) {                                                        \
    RETURN_ERROR(#name                                                         \
                 "() expects argument %d to class constructor as " NORMALIZE(  \
                     type) ", %s given",                                       \
                 (i) + 1, value_type(args[i]));                                  \
  }

#define EXCLUDE_ARG_TYPE(method_name, arg_type, index)                         \
  if (arg_type(args[index])) {                                                 \
    RETURN_ERROR("invalid type %s() as argument %d in %s()",                   \
                 value_type(args[index]), (index) + 1, #method_name);            \
  }

#define METHOD_OVERRIDE(override, i)                                           \
  do {                                                                         \
    if (IS_INSTANCE(args[0])) {                                                \
      b_obj_instance *instance = AS_INSTANCE(args[0]);                         \
      if (invoke_from_class(vm, instance->klass,                               \
                            copy_string(vm, "@" #override, (i) + 1), 0)) {               \
        args[-1] = TRUE_VAL;                                                   \
        return false; \
      }                                                                        \
    }                                                                          \
  } while (0);

#define REGEX_COMPILATION_ERROR(re, error_number, error_offset)                \
  if ((re) == NULL) {                                                            \
    PCRE2_UCHAR8 buffer[256];                                                  \
    pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));           \
    RETURN_ERROR("regular expression compilation failed at offset %d: %s",     \
                 (int)(error_offset), buffer);                                   \
  }

#define REGEX_ASSERTION_ERROR(re, match_data, ovector)                         \
  if ((ovector)[0] > (ovector)[1]) {                                               \
    RETURN_ERROR(                                                            \
        "match aborted: regular expression used \\K in an assertion %.*s to "  \
        "set match start after its end.",                                      \
        (int)((ovector)[0] - (ovector)[1]), (char *)(subject + (ovector)[1]));       \
    pcre2_match_data_free(match_data);                                         \
    pcre2_code_free(re);                                                       \
    RETURN_EMPTY;                                                          \
  }


#define REGEX_ERR(message, result) do { \
    PCRE2_UCHAR error[255]; \
    if(pcre2_get_error_message(result, error, 255)) { \
      RETURN_ERROR("RegexError: %s", (char*)error); \
    } \
    RETURN_ERROR("RegexError: %s", message);                \
  } while(0)

#define REGEX_RC_ERROR() REGEX_ERR("%d", rc);

#define GET_REGEX_COMPILE_OPTIONS(string, regex_show_error)              \
  uint32_t compile_options = is_regex(string);                                 \
  if ((regex_show_error) && (int)compile_options == -1) {                        \
    RETURN_ERROR("RegexError: Invalid regex");          \
  } else if ((regex_show_error) && (int)compile_options > 1000000) {                  \
    RETURN_ERROR("RegexError: invalid modifier '%c' ",       \
                 (char)abs(1000000 - (int)compile_options));                             \
  }


#define GC_STRING(o) OBJ_VAL(GC(copy_string(vm, (o), (int)strlen(o))))
#define GC_L_STRING(o, l) OBJ_VAL(GC(copy_string(vm, (o), (l))))
#define GC_T_STRING(o, l) OBJ_VAL(GC(take_string(vm, (o), (l))))
#define GC_TT_STRING(o) OBJ_VAL(GC(take_string(vm, (o), (int)strlen(o))))

extern uint32_t is_regex(b_obj_string *string);

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

DECLARE_NATIVE(typeof);

DECLARE_NATIVE(is_callable);

DECLARE_NATIVE(is_bool);

DECLARE_NATIVE(is_number);

DECLARE_NATIVE(is_int);

DECLARE_NATIVE(is_string);

DECLARE_NATIVE(is_bytes);

DECLARE_NATIVE(is_list);

DECLARE_NATIVE(is_dict);

DECLARE_NATIVE(is_object);

DECLARE_NATIVE(is_function);

DECLARE_NATIVE(is_class);

DECLARE_NATIVE(is_file);

DECLARE_NATIVE(is_instance);

DECLARE_NATIVE(is_iterable);

DECLARE_NATIVE(print);

#endif