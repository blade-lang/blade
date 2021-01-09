#ifndef bird_value_h
#define bird_value_h

#include "common.h"

typedef struct s_obj b_obj;
typedef struct s_obj_string b_obj_string;

typedef union {
  uint64_t bits;
  double num;
} b_double_union;

typedef enum {
  VAL_NIL,
  VAL_BOOL,
  VAL_NUMBER,
  VAL_OBJ,
  VAL_EMPTY,
} b_val_type;

typedef struct {
  b_val_type type;
  union {
    bool boolean;
    double number;
    b_obj *obj;
  } as;
} b_value;

// promote C values to bird value
#define EMPTY_VAL ((b_value){VAL_EMPTY, {.number = 0}})
#define NIL_VAL ((b_value){VAL_NIL, {.number = 0}})
#define BOOL_VAL(v) ((b_value){VAL_BOOL, {.boolean = v}})
#define NUMBER_VAL(v) ((b_value){VAL_NUMBER, {.number = v}})
#define OBJ_VAL(v) ((b_value){VAL_OBJ, {.obj = (b_obj *)v}})

// demote bird values to C value
#define AS_BOOL(v) ((v).as.boolean)
#define AS_NUMBER(v) ((v).as.number)
#define AS_OBJ(v) ((v).as.obj)

// testing bird value types
#define IS_NIL(v) ((v).type == VAL_NIL)
#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_OBJ(v) ((v).type == VAL_OBJ)
#define IS_EMPTY(v) ((v).type == VAL_EMPTY)

typedef struct {
  int capacity;
  int count;
  b_value *values;
} b_value_arr;

void init_value_arr(b_value_arr *array);
void free_value_arr(b_value_arr *array);
void write_value_arr(b_value_arr *array, b_value value);
void print_value(b_value value);
const char *value_type(b_value value);
bool values_equal(b_value a, b_value b);

// hash
uint32_t hash_string(const char *key, int length);
uint32_t hash_value(b_value value);

#endif