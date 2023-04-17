#include <blade.h>
#include <ffi.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#include <stdlib.h>

#else
#include "bdlfcn.h"
#endif

#define CLIB_RETURN_PTR(handle, cf, ...) \
  b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, (handle))); \
  const char *format = #cf; \
  int length = snprintf(NULL, 0, format, ##__VA_ARGS__); \
  ptr->name = ALLOCATE(char, length + 1); \
  sprintf((char *)ptr->name, format, ##__VA_ARGS__); \
  ptr->name[length] = '\0';   \
  RETURN_OBJ(ptr);

typedef struct {
  int length;
  int as_int;
  int *types;
  ffi_type *as_ffi;
} b_ffi_type;

#define DEFINE_CLIB_TYPE(v) \
  b_value __clib_type_##v(b_vm *vm) { \
    b_ffi_type *f = ALLOCATE(b_ffi_type, 1); \
    f->as_ffi = &ffi_type_##v;        \
    f->as_int = b_clib_type_##v;      \
    f->types = NULL;        \
    f->length = 0; \
    b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, (void *)f)); \
    const char *format = "%s";        \
    const char *str = "<void *clib::type::" #v ">"; \
    int length = snprintf(NULL, 0, format, str); \
    ptr->name = ALLOCATE(char, length + 1); \
    sprintf((char *)ptr->name, format, str); \
    ptr->name[length] = '\0';   \
    return OBJ_VAL(ptr); \
  }

#define DEFINE_CLIB_CTYPE(v) \
  b_value __clib_type_##v(b_vm *vm) { \
    b_ffi_type *f = ALLOCATE(b_ffi_type, 1); \
    f->as_ffi = &ffi_type_pointer;        \
    f->as_int = b_clib_type_##v;      \
    f->types = NULL;         \
    f->length = 0;\
    b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, (void *)f)); \
    const char *format = "%s";        \
    const char *str = "<void *clib::type::" #v ">"; \
    int length = snprintf(NULL, 0, format, str); \
    ptr->name = ALLOCATE(char, length + 1); \
    sprintf((char *)ptr->name, format, str); \
    ptr->name[length] = '\0';   \
    return OBJ_VAL(ptr); \
  }

#define GET_CLIB_TYPE(v) \
  {#v, true, __clib_type_##v}

typedef struct {
  bool is_variadic;
  unsigned int args_count;
  ffi_abi abi;
  ffi_cif *cif;
  b_ffi_type *return_type;
  b_ffi_type **arg_types;
  void *function;
} b_ffi_cif;

#define b_clib_type_void					  (-1)
#define b_clib_type_bool					  0
#define b_clib_type_uint8					  1
#define b_clib_type_sint8					  2
#define b_clib_type_uint16					3
#define b_clib_type_sint16					4
#define b_clib_type_uint32					5
#define b_clib_type_sint32					6
#define b_clib_type_uint64					7
#define b_clib_type_sint64					8
#define b_clib_type_float					  9
#define b_clib_type_double					10
#define b_clib_type_uchar					  11
#define b_clib_type_schar					  12
#define b_clib_type_ushort					13
#define b_clib_type_sshort					14
#define b_clib_type_uint					  15
#define b_clib_type_sint					  16
#define b_clib_type_ulong					  17
#define b_clib_type_slong					  18
#define b_clib_type_longdouble		  19
#define b_clib_type_char_ptr				20
#define b_clib_type_uchar_ptr			  21
#define b_clib_type_pointer					22
#define b_clib_type_struct					23

DEFINE_CLIB_TYPE(void);
DEFINE_CLIB_TYPE(uint8);
DEFINE_CLIB_TYPE(sint8);
DEFINE_CLIB_TYPE(uint16);
DEFINE_CLIB_TYPE(sint16);
DEFINE_CLIB_TYPE(uint32);
DEFINE_CLIB_TYPE(sint32);
DEFINE_CLIB_TYPE(uint64);
DEFINE_CLIB_TYPE(sint64);
DEFINE_CLIB_TYPE(float);
DEFINE_CLIB_TYPE(double);
DEFINE_CLIB_TYPE(uchar);
DEFINE_CLIB_TYPE(schar);
DEFINE_CLIB_TYPE(ushort);
DEFINE_CLIB_TYPE(sshort);
DEFINE_CLIB_TYPE(uint);
DEFINE_CLIB_TYPE(sint);
DEFINE_CLIB_TYPE(ulong);
DEFINE_CLIB_TYPE(slong);
DEFINE_CLIB_TYPE(longdouble);
DEFINE_CLIB_TYPE(pointer);
DEFINE_CLIB_CTYPE(char_ptr);
DEFINE_CLIB_CTYPE(uchar_ptr);

UNUSED b_value __clib_type_bool(b_vm *vm) {
  b_ffi_type *f = ALLOCATE(b_ffi_type, 1);
  f->as_ffi = &ffi_type_uint8;
  f->as_int = b_clib_type_bool;
  b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, (void *)f));
  const char *format = "%s";
  const char *str = "<void *clib::type::bool>";
  ptr->name = ALLOCATE(char, 25);
  sprintf((char *)ptr->name, format, str);
  ptr->name[24] = '\0';
  return OBJ_VAL(ptr);
}

#define CLIB_GET_C_VALUE(t) { \
    t* g = N_ALLOCATE(t, size);                          \
    *(t *)g = (t) AS_NUMBER(value); \
    return g; \
  }

#define CLIB_GET_C_CH_VALUE(t) {\
    t* g = N_ALLOCATE(t, size);                          \
    *(t *)g = (t) AS_C_STRING(value)[0]; \
    return g; \
  }

typedef struct {
  size_t count;
  size_t capacity;
  void **values;
} b_ffi_values;

static inline void add_value(b_vm *vm, b_ffi_values *values, void *object) {
  if (object == NULL)
    return;

  if (values->capacity < values->count + 1) {
    size_t old_capacity = values->capacity;
    values->capacity = GROW_CAPACITY(values->capacity);
    values->values = GROW_ARRAY(void *, values->values, old_capacity, values->capacity);

    if (values->values == NULL) {
      fflush(stdout); // flush out anything on stdout first
      fprintf(stderr, "CLib out of memory");
      exit(1);
    }
  }

  values->values[values->count++] = object;
}

static inline void *switch_c_values(b_vm *vm, int i, b_value value, size_t size) {
  switch(i) {
    case b_clib_type_bool: {
      bool* g = N_ALLOCATE(bool, size);
      *(bool *)g = AS_BOOL(value);
      return g;
    }
    case b_clib_type_uint8: CLIB_GET_C_VALUE(uint8_t);
    case b_clib_type_sint8: CLIB_GET_C_VALUE(int8_t);
    case b_clib_type_uint16: CLIB_GET_C_VALUE(uint16_t);
    case b_clib_type_sint16: CLIB_GET_C_VALUE(int16_t);
    case b_clib_type_uint32: CLIB_GET_C_VALUE(uint32_t);
    case b_clib_type_sint32: CLIB_GET_C_VALUE(int32_t);
    case b_clib_type_uint64: CLIB_GET_C_VALUE(uint64_t);
    case b_clib_type_sint64: CLIB_GET_C_VALUE(int64_t);
    case b_clib_type_float: CLIB_GET_C_VALUE(float);
    case b_clib_type_double: CLIB_GET_C_VALUE(double);
    case b_clib_type_uchar: CLIB_GET_C_CH_VALUE(unsigned char);
    case b_clib_type_schar: CLIB_GET_C_CH_VALUE(char);
    case b_clib_type_ushort: CLIB_GET_C_VALUE(unsigned short);
    case b_clib_type_sshort: CLIB_GET_C_VALUE(short);
    case b_clib_type_uint: CLIB_GET_C_VALUE(unsigned int);
    case b_clib_type_sint: CLIB_GET_C_VALUE(int);
    case b_clib_type_ulong: CLIB_GET_C_VALUE(unsigned long);
    case b_clib_type_slong: CLIB_GET_C_VALUE(long);
#ifdef LONG_LONG_MAX
    case b_clib_type_longdouble: CLIB_GET_C_VALUE(long long);
#else
    case b_clib_type_longdouble: CLIB_GET_C_VALUE(long);
#endif
    case b_clib_type_char_ptr: {
      char **v = N_ALLOCATE(char *, 1);
      if(IS_STRING(value)) {
        v[0] = AS_C_STRING(value);
      } else {
        v[0] = NULL;
      }
      return v;
    }
    case b_clib_type_uchar_ptr: {
      unsigned char **v = N_ALLOCATE(unsigned char *, size);
      if(IS_BYTES(value)) {
        v[0] = AS_BYTES(value)->bytes.bytes;
      } else {
        v[0] = NULL;
      }
      return v;
    }
    case b_clib_type_pointer: {
      void **v = N_ALLOCATE(void *, size);
      if(IS_PTR(value)) {
        v[0] = AS_PTR(value)->pointer;
      } else if(IS_FILE(value)) {
        v[0] = AS_FILE(value)->file;
      } else {
        v[0] = NULL;
      }
      return v;
    }
    case b_clib_type_struct: {
      if(IS_BYTES(value)) {
        b_obj_bytes *bytes = AS_BYTES(value);
        void *v = N_ALLOCATE(void, bytes->bytes.count);
        memcpy(v, bytes->bytes.bytes, bytes->bytes.count);
        return v;
      }
      return 0;
    }
    default: {
      break;
    }
  }

  return 0;
}

static inline b_ffi_values *get_c_values(b_vm *vm, b_ffi_cif *cif, b_obj_list *list) {
  b_ffi_values *values = ALLOCATE(b_ffi_values, 1);
  values->count = 0;
  values->capacity = 0;
  values->values = NULL;

  for(int i = 0; i < list->items.count; i++) {
    b_ffi_type *type = cif->arg_types[i];
    b_value value = list->items.values[i];

    void *v = switch_c_values(vm, type->as_int, value, type->as_ffi->size);
    add_value(vm, values, v);
  }

  return values;
}

DECLARE_MODULE_METHOD(clib_load_library) {
  ENFORCE_ARG_COUNT(load_library, 1);
  ENFORCE_ARG_TYPE(load_library, 0, IS_STRING);

  b_obj_string *name = AS_STRING(args[0]);

  void *handle;
  if((handle = dlopen(name->chars, RTLD_LAZY)) == NULL) {
    char *error = (char *)dlerror();
    RETURN_ERROR(error);
  }

  CLIB_RETURN_PTR(handle, <void *clib::Library(%s)>, name->chars);
}

DECLARE_MODULE_METHOD(clib_get_function) {
  ENFORCE_ARG_COUNT(get_function, 2);
  ENFORCE_ARG_TYPE(get_function, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get_function, 1, IS_STRING);

  void *handle = AS_PTR(args[0])->pointer;
  b_obj_string *name = AS_STRING(args[1]);

  if(handle) {
    void* fn = dlsym(handle, name->chars);
    if(fn == NULL) {
      char *error = (char *)dlerror();
      RETURN_ERROR(error);
    }

    CLIB_RETURN_PTR(fn, <void *clib::function(%s)>, name->chars);
  }

  RETURN_ERROR("handle not initialized");
}

DECLARE_MODULE_METHOD(clib_close_library) {
  ENFORCE_ARG_COUNT(close_library, 1);
  ENFORCE_ARG_TYPE(close_library, 0, IS_PTR);

  void *handle = AS_PTR(args[0])->pointer;

  if(handle) {
    dlclose(handle);
    RETURN;
  }

  RETURN_ERROR("handle not initialized");
}

DECLARE_MODULE_METHOD(clib_new_struct) {
  ENFORCE_ARG_COUNT(new_struct, 1);
  ENFORCE_ARG_TYPE(new_struct, 0, IS_LIST);

  b_obj_list *args_list = AS_LIST(args[0]);

  ffi_type *type = ALLOCATE(ffi_type, 1);
  ffi_type **elements = ALLOCATE(ffi_type *, args_list->items.count + 1);
  int *clib_types = ALLOCATE(int, args_list->items.count);

  for(int i = 0; i < args_list->items.count; i++) {
    b_ffi_type *t = AS_PTR(args_list->items.values[i])->pointer;
    clib_types[i] = t->as_int;
    elements[i] = t->as_ffi;
  }
  elements[args_list->items.count] = NULL;

  type->size = type->alignment = 0;
  type->type = FFI_TYPE_STRUCT;
  type->elements = elements;

  b_ffi_type *struct_type = ALLOCATE(b_ffi_type, 1);
  struct_type->as_int = b_clib_type_struct;
  struct_type->as_ffi = type;
  struct_type->types = clib_types;
  struct_type->length = args_list->items.count;

  CLIB_RETURN_PTR(struct_type, <void *clib::struct(%d)>, args_list->items.count);
}

DECLARE_MODULE_METHOD(clib_define) {
  ENFORCE_ARG_COUNT(define, 4);
  ENFORCE_ARG_TYPE(define, 0, IS_PTR);
  ENFORCE_ARG_TYPE(define, 1, IS_STRING);
  ENFORCE_ARG_TYPE(define, 2, IS_PTR);
  ENFORCE_ARG_TYPE(define, 3, IS_LIST);

  void *function = AS_PTR(args[0])->pointer;
  b_obj_string *fn_name = AS_STRING(args[1]);
  b_ffi_type *return_type = (b_ffi_type *)AS_PTR(args[2])->pointer;
  b_obj_list *args_list = AS_LIST(args[3]);

  if(function) {
    ffi_cif *cif = ALLOCATE(ffi_cif, 1);

    b_ffi_cif *ci = ALLOCATE(b_ffi_cif, 1);
    ci->function = function;
    ci->args_count = args_list->items.count;
    ci->abi = FFI_DEFAULT_ABI;
    ci->return_type = return_type;
    ci->is_variadic = false;
    ci->cif = cif;

    // populate the argument types...
    ci->arg_types = ALLOCATE(b_ffi_type *, args_list->items.count);
    ffi_type **types = ALLOCATE(ffi_type *, args_list->items.count + 1);

    // extract types out of b_ffi_type to ffi_type and into ci
    for (int i = 0; i < args_list->items.count; i++) {
      b_ffi_type *type = (b_ffi_type *) AS_PTR(args_list->items.values[i])->pointer;
      ci->arg_types[i] = type;
      types[i] = type->as_ffi;
    }
    types[args_list->items.count] = NULL;

    if(ffi_prep_cif(ci->cif, ci->abi, ci->args_count, ci->return_type->as_ffi, types) == FFI_OK) {
      CLIB_RETURN_PTR(ci, <void *clib::cif::%s(%d)>, fn_name->chars, ci->return_type->as_int);
    }

    RETURN_ERROR("failed to initialize call interface to %s()", fn_name->chars);
  }

  RETURN_ERROR("invalid function handle to %s()", fn_name->chars);
}

#define CLIB_CALL(t, r) {\
    t rc; \
    ffi_call(handle->cif, handle->function, &rc, values->values); \
    RETURN_##r(rc);                 \
  }

#define CLIB_STRUCT_VALUE(t, o) { \
    t v = *((t *)(data + total_size)); \
    write_list(vm, list, o(v));   \
    total_size += el->size > 0 ? el->size : sizeof(t); \
    break; \
  }

DECLARE_MODULE_METHOD(clib_call) {
  ENFORCE_ARG_COUNT(call, 2);
  ENFORCE_ARG_TYPE(call, 0, IS_PTR);
  ENFORCE_ARG_TYPE(call, 1, IS_LIST);

  b_ffi_cif *handle = (b_ffi_cif *)AS_PTR(args[0])->pointer;
  if(handle) {
    b_obj_list *args_list = AS_LIST(args[1]);
    if(args_list->items.count > handle->args_count && !handle->is_variadic) {
      RETURN_ERROR("invalid number of arguments");
    }

    b_ffi_values *values = get_c_values(vm, handle, args_list);

    switch (handle->return_type->as_int) {
      case b_clib_type_void: {
        ffi_call(handle->cif, handle->function, NULL, values->values);
        RETURN;
      }
      case b_clib_type_bool: {
        int b;
        ffi_call(handle->cif, handle->function, &b, values->values);
        RETURN_BOOL(b > 0);
      }
      case b_clib_type_uint8: CLIB_CALL(uint8_t, NUMBER);
      case b_clib_type_sint8: CLIB_CALL(int8_t, NUMBER);
      case b_clib_type_uint16: CLIB_CALL(uint16_t, NUMBER);
      case b_clib_type_sint16: CLIB_CALL(int16_t, NUMBER);
      case b_clib_type_uint32: CLIB_CALL(uint32_t, NUMBER);
      case b_clib_type_sint32: CLIB_CALL(int32_t, NUMBER);
      case b_clib_type_uint64: CLIB_CALL(uint64_t, NUMBER);
      case b_clib_type_sint64: CLIB_CALL(int64_t, NUMBER);
      case b_clib_type_float: CLIB_CALL(float, NUMBER);
      case b_clib_type_double: CLIB_CALL(double, NUMBER);
      case b_clib_type_uchar: CLIB_CALL(unsigned char, NUMBER);
      case b_clib_type_schar: CLIB_CALL(char, NUMBER);
      case b_clib_type_ushort: CLIB_CALL(unsigned short, NUMBER);
      case b_clib_type_sshort: CLIB_CALL(short, NUMBER);
      case b_clib_type_uint: CLIB_CALL(unsigned int, NUMBER);
      case b_clib_type_sint: CLIB_CALL(int, NUMBER);
      case b_clib_type_ulong: CLIB_CALL(unsigned long, NUMBER);
      case b_clib_type_slong: CLIB_CALL(long, NUMBER);
#ifdef LONG_LONG_MAX
      case b_clib_type_longdouble: CLIB_CALL(long long, NUMBER);
#else
      case b_clib_type_longdouble: CLIB_CALL(long, NUMBER);
#endif
      case b_clib_type_char_ptr: CLIB_CALL(char *, STRING);
      case b_clib_type_uchar_ptr:
      case b_clib_type_struct: {
        ffi_arg result;
        ffi_call(handle->cif, handle->function, &result, values->values);
        b_obj_bytes *bytes = (b_obj_bytes *)GC(new_bytes(vm, handle->cif->rtype->size));
        memcpy(bytes->bytes.bytes, &result, handle->cif->rtype->size);
        RETURN_OBJ(bytes);
      }
      case b_clib_type_pointer: CLIB_CALL(void *, PTR);
      default: RETURN;
    }
  }

  RETURN_ERROR("function handle not initialized");
}

CREATE_MODULE_LOADER(clib) {
  static b_field_reg module_fields[] = {
      GET_CLIB_TYPE(void),
      GET_CLIB_TYPE(bool),
      GET_CLIB_TYPE(uint8),
      GET_CLIB_TYPE(sint8),
      GET_CLIB_TYPE(uint16),
      GET_CLIB_TYPE(sint16),
      GET_CLIB_TYPE(uint32),
      GET_CLIB_TYPE(sint32),
      GET_CLIB_TYPE(uint64),
      GET_CLIB_TYPE(sint64),
      GET_CLIB_TYPE(float),
      GET_CLIB_TYPE(double),
      GET_CLIB_TYPE(uchar),
      GET_CLIB_TYPE(schar),
      GET_CLIB_TYPE(ushort),
      GET_CLIB_TYPE(sshort),
      GET_CLIB_TYPE(uint),
      GET_CLIB_TYPE(sint),
      GET_CLIB_TYPE(ulong),
      GET_CLIB_TYPE(slong),
      GET_CLIB_TYPE(longdouble),
      GET_CLIB_TYPE(pointer),
      GET_CLIB_TYPE(char_ptr),
      GET_CLIB_TYPE(uchar_ptr),
      {NULL, false, NULL}
  };

  static b_func_reg module_functions[] = {
      {"load",   true,  GET_MODULE_METHOD(clib_load_library)},
      {"function",   true,  GET_MODULE_METHOD(clib_get_function)},
      {"close",   true,  GET_MODULE_METHOD(clib_close_library)},
      {"define",   true,  GET_MODULE_METHOD(clib_define)},
      {"call",   true,  GET_MODULE_METHOD(clib_call)},
      {"new_struct",   true,  GET_MODULE_METHOD(clib_new_struct)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_clib",
      .fields = module_fields,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}

#undef CLIB_STRUCT_VALUE
#undef DEFINE_CLIB_TYPE
#undef GET_CLIB_TYPE
#undef CLIB_CALL
#undef CLIB_GET_C_VALUE
#undef CLIB_GET_C_CH_VALUE
#undef CLIB_RETURN_PTR
