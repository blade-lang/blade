#include <stdio.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "object.h"
#include "value.h"

void init_value_arr(b_value_arr *array) {
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void write_value_arr(b_vm *vm, b_value_arr *array, b_value value) {
  if (array->capacity < array->count + 1) {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values =
        GROW_ARRAY(b_value, array->values, old_capacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

void free_value_arr(b_vm *vm, b_value_arr *array) {
  FREE_ARRAY(b_value, array->values, array->capacity);
  init_value_arr(array);
}

void print_value(b_value value) {
  switch (value.type) {
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  case VAL_NUMBER:
    printf(NUMBER_FORMAT, AS_NUMBER(value));
    break;
  case VAL_OBJ:
    print_object(value);
    break;

  default:
    break;
  }
}

const char *value_type(b_value value) {
  if (IS_NIL(value))
    return "nil";
  else if (IS_BOOL(value))
    return "boolean";
  else if (IS_NUMBER(value))
    return "number";
  else if (IS_OBJ(value))
    return object_type(AS_OBJ(value));
  else
    return "unknown";
}

bool values_equal(b_value a, b_value b) {
  if (a.type != b.type)
    return false;

  switch (a.type) {
  case VAL_NIL:
    return true;
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJ:
    return AS_OBJ(a) == AS_OBJ(b);

  default:
    return false;
  }
}

static inline uint32_t hash_bits(uint64_t hash) {
  // From v8's ComputeLongHash() which in turn cites:
  // Thomas Wang, Integer Hash Functions.
  // http://www.concentric.net/~Ttwang/tech/inthash.htm
  hash = ~hash + (hash << 18); // hash = (hash << 18) - hash - 1;
  hash = hash ^ (hash >> 31);
  hash = hash * 21; // hash = (hash + (hash << 2)) + (hash << 4);
  hash = hash ^ (hash >> 11);
  hash = hash + (hash << 6);
  hash = hash ^ (hash >> 22);
  return (uint32_t)(hash & 0x3fffffff);
}

uint32_t hash_double(double value) {
  b_double_union bits;
  bits.num = value;
  return hash_bits(bits.bits);
}

uint32_t hash_string(const char *key, int length) {
  uint32_t hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}

// Generates a hash code for [object].
static uint32_t hash_object(b_obj *object) {
  switch (object->type) {
    /* case OBJ_CLASS:
      // Classes just use their name.
      return hash_object((b_obj *)((b_obj_class *)object)->name);

      // Allow bare (non-closure) functions so that we can use a map to find
      // existing constants in a function's constant table. This is only used
      // internally. Since user code never sees a non-closure function, they
      // cannot use them as map keys.
    case OBJ_FUNCTION: {
      b_obj_func *fn = (b_obj_func *)object;
      return hash_double(fn->arity) ^ hash_double(fn->blob.count);
    } */

  case OBJ_STRING:
    return ((b_obj_string *)object)->hash;

  default:
    return 0;
  }
}

uint32_t hash_value(b_value value) {
  /* #ifdef NAN_BOXING
    if (IS_OBJ(value))
      return hash_object(AS_OBJ(value));
    return hash_bits(value);
  #else */
  switch (value.type) {
  case VAL_BOOL:
    return AS_BOOL(value) ? 3 : 5;

  case VAL_NIL:
    return 7;

  case VAL_NUMBER:
    return hash_double(AS_NUMBER(value));

  case VAL_OBJ:
    return hash_object(AS_OBJ(value));

  default: // VAL_EMPTY
    return 0;
  }
  /* #endif */
}