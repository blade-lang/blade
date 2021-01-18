#ifndef bird_object_h
#define bird_object_h

#include "blob.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

// object type checks
#define IS_STRING(v) is_obj_type(v, OBJ_STRING)
#define IS_NATIVE(v) is_obj_type(v, OBJ_NATIVE)
#define IS_FUNCTION(v) is_obj_type(v, OBJ_FUNCTION)
#define IS_CLOSURE(v) is_obj_type(v, OBJ_CLOSURE)
#define IS_CLASS(v) is_obj_type(v, OBJ_CLASS)
#define IS_INSTANCE(v) is_obj_type(v, OBJ_INSTANCE)

// promote b_value to object
#define AS_STRING(v) ((b_obj_string *)AS_OBJ(v))
#define AS_NATIVE(v) ((b_obj_native *)AS_OBJ(v))
#define AS_FUNCTION(v) ((b_obj_func *)AS_OBJ(v))
#define AS_CLOSURE(v) ((b_obj_closure *)AS_OBJ(v))
#define AS_CLASS(v) ((b_obj_class *)AS_OBJ(v))
#define AS_INSTANCE(v) ((b_obj_instance *)AS_OBJ(v))

// demote bird value to c string
#define AS_CSTRING(v) (((b_obj_string *)AS_OBJ(v))->chars)

typedef enum {
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
} b_obj_type;

struct s_obj {
  b_obj_type type;
  bool is_marked;
  struct s_obj *next;
};

struct s_obj_string {
  b_obj obj;
  int length;
  char *chars;
  uint32_t hash;
};

typedef struct b_obj_upvalue {
  b_obj obj;
  b_value *location;
  b_value closed;
  struct b_obj_upvalue *next;
} b_obj_upvalue;

typedef struct {
  b_obj obj;
  int arity;
  int upvalue_count;
  b_blob blob;
  b_obj_string *name;
} b_obj_func;

typedef struct {
  b_obj obj;
  b_obj_func *function;
  b_obj_upvalue **upvalues;
  int upvalue_count;
} b_obj_closure;

typedef struct {
  b_obj obj;
  b_obj_string *name;
  b_table fields;
  b_table methods;
} b_obj_class;

typedef struct {
  b_obj obj;
  b_obj_class *klass;
  b_table fields;
} b_obj_instance;

typedef b_value (*b_native_fn)(b_vm *, int, b_value *);

typedef struct {
  b_obj obj;
  const char *name;
  b_native_fn function;
} b_obj_native;

b_obj_class *new_class(b_vm *vm, b_obj_string *name);
b_obj_closure *new_closure(b_vm *vm, b_obj_func *function);
b_obj_func *new_function(b_vm *vm);
b_obj_instance *new_instance(b_vm *vm, b_obj_class *klass);
b_obj_upvalue *new_upvalue(b_vm *vm, b_value *slot);
b_obj_native *new_native(b_vm *vm, b_native_fn function, const char *name);
b_obj_string *copy_string(b_vm *vm, const char *chars, int length);
b_obj_string *take_string(b_vm *vm, char *chars, int length);
void print_object(b_value value);
const char *object_type(b_obj *object);

static inline bool is_obj_type(b_value v, b_obj_type t) {
  return IS_OBJ(v) && AS_OBJ(v)->type == t;
}

#endif