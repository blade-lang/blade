#ifndef bird_object_h
#define bird_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

// object type checks
#define IS_STRING(v) is_obj_type(v, OBJ_STRING)

// promote b_value to object
#define AS_STRING(v) ((b_obj_string *)AS_OBJ(v))

// demote bird value to c string
#define AS_CSTRING(v) (((b_obj_string *)AS_OBJ(v))->chars)

typedef enum {
  OBJ_STRING,
} b_obj_type;

struct s_obj {
  b_obj_type type;
  struct s_obj *next;
};

struct s_obj_string {
  b_obj obj;
  int length;
  char *chars;
  uint32_t hash;
};

b_obj_string *copy_string(b_vm *vm, const char *chars, int length);
b_obj_string *take_string(b_vm *vm, char *chars, int length);
void print_object(b_value value);

static inline bool is_obj_type(b_value v, b_obj_type t) {
  return IS_OBJ(v) && AS_OBJ(v)->type == t;
}

#endif