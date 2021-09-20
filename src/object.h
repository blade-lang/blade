#ifndef BLADE_OBJECT_H
#define BLADE_OBJECT_H

#include "blob.h"
#include "common.h"
#include "table.h"
#include "value.h"

#include <stdio.h>

typedef enum {
  TYPE_FUNCTION,
  TYPE_METHOD,
  TYPE_INITIALIZER,
  TYPE_PRIVATE,
  TYPE_STATIC,
  TYPE_SCRIPT,
} b_func_type;

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

// object type checks
#define IS_STRING(v) is_obj_type(v, OBJ_STRING)
#define IS_NATIVE(v) is_obj_type(v, OBJ_NATIVE)
#define IS_FUNCTION(v) is_obj_type(v, OBJ_FUNCTION)
#define IS_CLOSURE(v) is_obj_type(v, OBJ_CLOSURE)
#define IS_CLASS(v) is_obj_type(v, OBJ_CLASS)
#define IS_INSTANCE(v) is_obj_type(v, OBJ_INSTANCE)
#define IS_BOUND(v) is_obj_type(v, OBJ_BOUND_METHOD)

// containers
#define IS_BYTES(v) is_obj_type(v, OBJ_BYTES)
#define IS_LIST(v) is_obj_type(v, OBJ_LIST)
#define IS_DICT(v) is_obj_type(v, OBJ_DICT)
#define IS_FILE(v) is_obj_type(v, OBJ_FILE)
#define IS_RANGE(v) is_obj_type(v, OBJ_RANGE)

// promote b_value to object
#define AS_STRING(v) ((b_obj_string *)AS_OBJ(v))
#define AS_NATIVE(v) ((b_obj_native *)AS_OBJ(v))
#define AS_FUNCTION(v) ((b_obj_func *)AS_OBJ(v))
#define AS_CLOSURE(v) ((b_obj_closure *)AS_OBJ(v))
#define AS_CLASS(v) ((b_obj_class *)AS_OBJ(v))
#define AS_INSTANCE(v) ((b_obj_instance *)AS_OBJ(v))
#define AS_BOUND(v) ((b_obj_bound *)AS_OBJ(v))

// non-user objects
#define AS_SWITCH(v) ((b_obj_switch *)AS_OBJ(v))
#define IS_SWITCH(v) is_obj_type(v, OBJ_SWITCH)
#define AS_PTR(v) ((b_obj_ptr *)AS_OBJ(v))
#define IS_PTR(v) is_obj_type(v, OBJ_PTR)
#define AS_MODULE(v) ((b_obj_module *)AS_OBJ(v))
#define IS_MODULE(v) is_obj_type(v, OBJ_MODULE)

// containers
#define AS_BYTES(v) ((b_obj_bytes *)AS_OBJ(v))
#define AS_LIST(v) ((b_obj_list *)AS_OBJ(v))
#define AS_DICT(v) ((b_obj_dict *)AS_OBJ(v))
#define AS_FILE(v) ((b_obj_file *)AS_OBJ(v))
#define AS_RANGE(v) ((b_obj_range *)AS_OBJ(v))

// demote blade value to c string
#define AS_C_STRING(v) (((b_obj_string *)AS_OBJ(v))->chars)

#define IS_CHAR(v) (IS_STRING(v) && (AS_STRING(v)->length == 1 || AS_STRING(v)->length == 0))

typedef enum {
  // containers
  OBJ_STRING,
  OBJ_RANGE,
  OBJ_LIST,
  OBJ_DICT,
  OBJ_FILE,
  OBJ_BYTES,

  // base object types
  OBJ_UP_VALUE,
  OBJ_BOUND_METHOD,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_CLASS,

  // non-user objects
  OBJ_MODULE,
  OBJ_SWITCH,
  OBJ_PTR,  // object type that can hold any C pointer
} b_obj_type;

struct s_obj {
  b_obj_type type;
  bool mark;
  struct s_obj *next;
};

struct s_obj_string {
  b_obj obj;
  int length;
  int utf8_length;
  uint32_t hash;
  char *chars;
};

typedef struct b_obj_up_value {
  b_obj obj;
  b_value closed;
  b_value *location;
  struct b_obj_up_value *next;
} b_obj_up_value;

typedef struct {
  b_obj obj;
  bool imported;
  b_table values;
  char *name;
  char *file;
  void *preloader;
  void *unloader;
  void *handle;
} b_obj_module;

typedef struct {
  b_obj obj;
  b_func_type type;
  int arity;
  int up_value_count;
  bool is_variadic;
  b_blob blob;
  b_obj_string *name;
  b_obj_module *module;
} b_obj_func;

typedef struct {
  b_obj obj;
  int up_value_count;
  b_obj_func *function;
  b_obj_up_value **up_values;
} b_obj_closure;

typedef struct b_obj_class {
  b_obj obj;
  b_value initializer;
  b_table properties;
  b_table static_properties;
  b_table methods;
  b_obj_string *name;
  struct b_obj_class *superclass;
} b_obj_class;

typedef struct {
  b_obj obj;
  b_table properties;
  b_obj_class *klass;
} b_obj_instance;

typedef struct {
  b_obj obj;
  b_value receiver;
  b_obj_closure *method;
} b_obj_bound; // a bound method

typedef bool (*b_native_fn)(b_vm *, int, b_value *);

typedef struct b_obj_native {
  b_obj obj;
  b_func_type type;
  const char *name;
  b_native_fn function;
} b_obj_native;

typedef struct {
  b_obj obj;
  b_value_arr items;
} b_obj_list;

typedef struct {
  b_obj obj;
  int lower;
  int upper;
  int range;
} b_obj_range;

typedef struct {
  b_obj obj;
  b_byte_arr bytes;
} b_obj_bytes;

typedef struct {
  b_obj obj;
  b_value_arr names;
  b_table items;
} b_obj_dict;

typedef struct {
  b_obj obj;
  bool is_open;
  FILE *file;
  b_obj_string *mode;
  b_obj_string *path;
} b_obj_file;

typedef struct {
  b_obj obj;
  int default_jump;
  int exit_jump;
  b_table table;
} b_obj_switch;

typedef struct {
  b_obj obj;
  void *pointer;
  const char *name;
} b_obj_ptr;

// non-user objects...
b_obj_module *new_module(b_vm *vm, char *name, char *file);

b_obj_switch *new_switch(b_vm *vm);
b_obj_ptr *new_ptr(b_vm *vm, void *pointer);

// data containers
b_obj_list *new_list(b_vm *vm);
b_obj_range *new_range(b_vm *vm, int lower, int upper);

b_obj_bytes *new_bytes(b_vm *vm, int length);

b_obj_dict *new_dict(b_vm *vm);

b_obj_file *new_file(b_vm *vm, b_obj_string *path, b_obj_string *mode);

// base objects
b_obj_bound *new_bound_method(b_vm *vm, b_value receiver, b_obj_closure *method);

b_obj_class *new_class(b_vm *vm, b_obj_string *name);

b_obj_closure *new_closure(b_vm *vm, b_obj_func *function);

b_obj_func *new_function(b_vm *vm, b_obj_module *module, b_func_type type);

b_obj_instance *new_instance(b_vm *vm, b_obj_class *klass);

b_obj_up_value *new_up_value(b_vm *vm, b_value *slot);

b_obj_native *new_native(b_vm *vm, b_native_fn function, const char *name);

b_obj_string *copy_string(b_vm *vm, const char *chars, int length);

b_obj_string *take_string(b_vm *vm, char *chars, int length);

void print_object(b_value value, bool fix_string);

const char *object_type(b_obj *object);

char *object_to_string(b_vm *vm, b_value value);

b_obj_bytes *copy_bytes(b_vm *vm, unsigned char *b, int length);

b_obj_bytes *take_bytes(b_vm *vm, unsigned char *b, int length);

static inline bool is_obj_type(b_value v, b_obj_type t) {
  return IS_OBJ(v) && AS_OBJ(v)->type == t;
}

static inline bool is_std_file(b_obj_file *file) { return file->mode->length == 0; }

#endif