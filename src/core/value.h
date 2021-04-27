#ifndef bird_value_h
#define bird_value_h

#include <string.h>

#include "common.h"

typedef struct s_obj b_obj;
typedef struct s_obj_string b_obj_string;
typedef struct s_vm b_vm;

typedef union {
  uint64_t bits;
  double num;
} b_double_union;

#if defined USE_NAN_BOXING && USE_NAN_BOXING

// binary representation = 1111111111111 i.e.
// 11 bits + 1 bit for quite nan and another
// bit to dodge intel's QNaN Floating-Point Indefinite bit
#define QNAN ((uint64_t)0x7ffc000000000000)

#define SIGN_BIT ((uint64_t)0x8000000000000000)

#define EMPTY_TAG 0 // 00
#define NIL_TAG 1   // 01
#define FALSE_TAG 2 // 10
#define TRUE_TAG 3  // 11

typedef uint64_t b_value;

#define FALSE_VAL ((b_value)(uint64_t)(QNAN | FALSE_TAG))
#define TRUE_VAL ((b_value)(uint64_t)(QNAN | TRUE_TAG))

#define EMPTY_VAL ((b_value)(uint64_t)(QNAN | EMPTY_TAG))
#define NIL_VAL ((b_value)(uint64_t)(QNAN | NIL_TAG))
#define BOOL_VAL(v) ((v) ? TRUE_VAL : FALSE_VAL)
#define NUMBER_VAL(v) number_to_value(v)
#define INTEGER_VAL(v) integer_to_value(v)
#define OBJ_VAL(obj) (b_value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

#define AS_BOOL(v) ((v) == TRUE_VAL)
#define AS_NUMBER(v) value_to_number(v)
#define AS_OBJ(v) ((b_obj *)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))

#define IS_EMPTY(v) ((v) == EMPTY_VAL)
#define IS_NIL(v) ((v) == NIL_VAL)
#define IS_BOOL(v) (((v) | 1) == TRUE_VAL)
#define IS_NUMBER(v) (((v)&QNAN) != QNAN)
#define IS_OBJ(v) (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

static inline b_value number_to_value(double v) {
  b_value value;
  memcpy(&value, &v, sizeof(double));
  return value;
}

static inline b_value integer_to_value(int v) {
  b_double_union data;
  data.num = (double) v;
  return data.bits;
}

static inline double value_to_number(b_value v) {
  double number;
  memcpy(&number, &v, sizeof(b_value));
  return number;
}

#else

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
#define TRUE_VAL ((b_value){VAL_BOOL, {.boolean = true}})
#define FALSE_VAL ((b_value){VAL_BOOL, {.boolean = false}})
#define BOOL_VAL(v) ((b_value){VAL_BOOL, {.boolean = v}})
#define NUMBER_VAL(v) ((b_value){VAL_NUMBER, {.number = v}})
#define INTEGER_VAL(v) ((b_value){VAL_NUMBER, {.number = v}})
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

#endif

typedef struct {
  int capacity;
  int count;
  b_value *values;
} b_value_arr;

typedef struct {
  int count;
  unsigned char *bytes;
} b_byte_arr;

void init_value_arr(b_value_arr *array);

void free_value_arr(b_vm *vm, b_value_arr *array);

void write_value_arr(b_vm *vm, b_value_arr *array, b_value value);

void insert_value_arr(b_vm *vm, b_value_arr *array, b_value value, int index);

void print_value(b_value value);

void echo_value(b_value value);

const char *value_type(b_value value);

bool values_equal(b_value a, b_value b);

char *value_to_string(b_vm *vm, b_value value);

void init_byte_arr(b_byte_arr *array, int length);

void free_byte_arr(b_vm *vm, b_byte_arr *array);

// hash
uint32_t hash_string(const char *key, int length);

uint32_t hash_value(b_value value);

void sort_values(b_value *values, int count);

#define STRING_VAL(val) OBJ_VAL(copy_string(vm, val, (int)strlen(val)))
#define STRING_L_VAL(val, l) OBJ_VAL(copy_string(vm, val, l))

#endif