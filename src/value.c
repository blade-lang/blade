#include "value.h"
#include "config.h"
#include "memory.h"
#include "object.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_value_arr(b_value_arr *array) {
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void init_byte_arr(b_byte_arr *array, int length) {
  array->count = length;
  array->bytes = (unsigned char *) calloc(length, sizeof(unsigned char));
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

void insert_value_arr(b_vm *vm, b_value_arr *array, b_value value, int index) {

  if (array->capacity <= index) {
    array->capacity = GROW_CAPACITY(index);
    array->values =
        GROW_ARRAY(b_value, array->values, array->count, array->capacity);
  } else if (array->capacity < array->count + 2) {
    int capacity = array->capacity;
    array->capacity = GROW_CAPACITY(capacity);
    array->values =
        GROW_ARRAY(b_value, array->values, capacity, array->capacity);
  }

  if (index <= array->count) {
    for (int i = array->count - 1; i >= index; i--) {
      array->values[i + 1] = array->values[i];
    }
  } else {
    for (int i = array->count; i < index; i++) {
      array->values[i] = NIL_VAL; // nil out overflow indices
      array->count++;
    }
  }

  array->values[index] = value;
  array->count++;
}

void free_value_arr(b_vm *vm, b_value_arr *array) {
  FREE_ARRAY(b_value, array->values, array->capacity);
  init_value_arr(array);
}

void free_byte_arr(b_vm *vm, b_byte_arr *array) {
  FREE_ARRAY(unsigned char, array->bytes, array->count);
}

static inline void do_print_value(b_value value, bool fix_string) {
#if defined(USE_NAN_BOXING) && USE_NAN_BOXING
  if (IS_EMPTY(value)) return;
  else if (IS_NIL(value))
    printf("nil");
  else if (IS_BOOL(value))
    printf(AS_BOOL(value) ? "true" : "false");
  else if (IS_NUMBER(value))
    printf(NUMBER_FORMAT, AS_NUMBER(value));
  else
    print_object(value, fix_string);
#else
  switch (value.type) {
  case VAL_EMPTY:
    break;
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
    print_object(value, fix_string);
    break;

  default:
    break;
  }
#endif
}

#ifndef _WIN32

inline void print_value(b_value value) { do_print_value(value, false); }

inline void echo_value(b_value value) { do_print_value(value, true); }

#else
void print_value(b_value value) { do_print_value(value, false); }
void echo_value(b_value value) { do_print_value(value, true); }
#endif // !_WIN32

static inline char *number_to_string(double number) {
  int length = snprintf(NULL, 0, NUMBER_FORMAT, number);
  char *num_str = (char *) calloc((size_t) length + 1, sizeof(char));
  if (num_str != NULL) {
    sprintf(num_str, NUMBER_FORMAT, number);
    return num_str;
  }
  return "";
}

char *value_to_string(b_vm *vm, b_value value) {
#if defined(USE_NAN_BOXING) && USE_NAN_BOXING
  if (IS_EMPTY(value))
    return strdup("");
  if (IS_NIL(value))
    return strdup("nil");
  else if (IS_BOOL(value))
    return strdup(AS_BOOL(value) ? "true" : "false");
  else if (IS_NUMBER(value))
    return number_to_string(AS_NUMBER(value));
  else
    return object_to_string(vm, value);
#else
  switch (value.type) {
  case VAL_NIL:
    return "nil";
  case VAL_BOOL:
    return AS_BOOL(value) ? "true" : "false";
  case VAL_NUMBER:
    return number_to_string(AS_NUMBER(value));
  case VAL_OBJ:
    return object_to_string(vm, value);

  default:
    return "";
  }
#endif
}

const char *value_type(b_value value) {
  if (IS_EMPTY(value))
    return "empty";
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
#if defined(USE_NAN_BOXING) && USE_NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b))
    return AS_NUMBER(a) == AS_NUMBER(b);
  return a == b;
#else
  if (a.type != b.type)
    return false;

  switch (a.type) {
  case VAL_NIL:
  case VAL_EMPTY:
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
#endif
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
  return (uint32_t) (hash & 0x3fffffff);
}

uint32_t hash_double(double value) {
  b_double_union bits;
  bits.num = value;
  return hash_bits(bits.bits);
}

/* uint32_t inline hash_string(const char *key, int length) {
  uint32_t hash = 0;

  for (int i = 0, j = length - 1; i < length; i++, j--) {
    hash += key[i] * 92821 ^ j;
  }

  return hash;
} */

/* #if defined(SUPPORT_LITTLE_ENDIAN) && SUPPORT_LITTLE_ENDIAN == 1
#define _le64toh(x) ((uint64_t)(x))
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define _le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(HAVE_LETOH64)
#if defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#else
#include <endian.h>
#endif
#define _le64toh(x) le64toh(x)
#else
#define _le64toh(x)                                                            \
  (((uint64_t)(x) << 56) | (((uint64_t)(x) << 40) & 0xff000000000000ULL) |     \
   (((uint64_t)(x) << 24) & 0xff0000000000ULL) |                               \
   (((uint64_t)(x) << 8) & 0xff00000000ULL) |                                  \
   (((uint64_t)(x) >> 8) & 0xff000000ULL) |                                    \
   (((uint64_t)(x) >> 24) & 0xff0000ULL) |                                     \
   (((uint64_t)(x) >> 40) & 0xff00ULL) | ((uint64_t)(x) >> 56))
#endif

#ifdef _MSC_VER
#define ROTATE(x, b) _rotl64(x, b)
#else
#define ROTATE(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#endif

#define HALF_ROUND(a, b, c, d, s, t)                                           \
  a += b;                                                                      \
  c += d;                                                                      \
  b = ROTATE(b, s) ^ a;                                                        \
  d = ROTATE(d, t) ^ c;                                                        \
  a = ROTATE(a, 32);

#define DOUBLE_ROUND(v0, v1, v2, v3)                                           \
  HALF_ROUND(v0, v1, v2, v3, 13, 16);                                          \
  HALF_ROUND(v2, v1, v0, v3, 17, 21);                                          \
  HALF_ROUND(v0, v1, v2, v3, 13, 16);                                          \
  HALF_ROUND(v2, v1, v0, v3, 17, 21);

static uint64_t siphash24(uint64_t k0, uint64_t k1, const char *src,
                          int src_sz) {
  uint64_t b = (uint64_t)src_sz << 56;
  const uint64_t *in = (uint64_t *)src;

  uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
  uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
  uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
  uint64_t v3 = k1 ^ 0x7465646279746573ULL;

  uint64_t t;
  uint8_t *pt;
  uint8_t *m;

  while (src_sz >= 8) {
    uint64_t mi = _le64toh(*in);
    in += 1;
    src_sz -= 8;
    v3 ^= mi;
    DOUBLE_ROUND(v0, v1, v2, v3);
    v0 ^= mi;
  }

  t = 0;
  pt = (uint8_t *)&t;
  m = (uint8_t *)in;
  switch (src_sz) {
  case 7:
    pt[6] = m[6]; // fall through
  case 6:
    pt[5] = m[5]; // fall through
  case 5:
    pt[4] = m[4]; // fall through
  case 4:
    memcpy(pt, m, sizeof(uint32_t));
    break;
  case 3:
    pt[2] = m[2]; // fall through
  case 2:
    pt[1] = m[1]; // fall through
  case 1:
    pt[0] = m[0]; // fall through
  }
  b |= _le64toh(t);

  v3 ^= b;
  DOUBLE_ROUND(v0, v1, v2, v3);
  v0 ^= b;
  v2 ^= 0xff;
  DOUBLE_ROUND(v0, v1, v2, v3);
  DOUBLE_ROUND(v0, v1, v2, v3);

  // modified
  t = (v0 ^ v1) ^ (v2 ^ v3);
  return t;
} */

#ifndef _WIN32

inline uint32_t hash_string(const char *key, int length) {
#else
uint32_t hash_string(const char *key, int length) {
#endif // !_WIN32

  uint32_t hash = 2166136261u;
  const char *be = key + length;

  while (key < be) {
    hash = (hash ^ *key++) * 16777619;
  }

  return hash;
  // return siphash24(127, 255, key, length);
}

/*#define _PADr_KAZE(x, n) ( ((x) << (n))>>(n) )
uint32_t hash_string(const char *str, int wrdlen) {
  const uint32_t PRIME = 591798841; uint32_t hash32;
  uint64_t hash64 = 14695981039346656037u; const char *p = str;
  int i, Cycles, NDhead;
  if (wrdlen > 8) {
    Cycles = ((wrdlen - 1)>>4) + 1; NDhead = wrdlen - (Cycles<<3);
    for(i=0; i<Cycles; i++) {
      hash64 = ( hash64 ^ (*(uint64_t *)(p)) ) * PRIME;
      hash64 = ( hash64 ^ (*(uint64_t *)(p+NDhead)) ) * PRIME;
      p += 8;
    }
  } else {
    hash64 = (hash64 ^ _PADr_KAZE(*(uint64_t *) (p + 0), (8 - wrdlen) << 3)) * PRIME;
  }
  hash32 = (uint32_t)(hash64 ^ (hash64>>32)); return hash32 ^ (hash32 >> 16);
}*/

// Generates a hash code for [object].
static uint32_t hash_object(b_obj *object) {
  switch (object->type) {
    case OBJ_CLASS:
      // Classes just use their name.
      return hash_object((b_obj *) ((b_obj_class *) object)->name);

      // Allow bare (non-closure) functions so that we can use a map to find
      // existing constants in a function's constant table. This is only used
      // internally. Since user code never sees a non-closure function, they
      // cannot use them as map keys.
    case OBJ_FUNCTION: {
      b_obj_func *fn = (b_obj_func *) object;
      return hash_double(fn->arity) ^ hash_double(fn->blob.count);
    }

    case OBJ_STRING:
      return ((b_obj_string *) object)->hash;

    default:
      return 0;
  }
}

uint32_t hash_value(b_value value) {
#if defined(USE_NAN_BOXING) && USE_NAN_BOXING
  if (IS_OBJ(value))
    return hash_object(AS_OBJ(value));
  return hash_bits(value);
#else
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
#endif
}

/**
 * returns the greater of the two values.
 * this function encapsulates Blade's object hierarchy
 */
static b_value find_max_value(b_value a, b_value b) {
  if (IS_NIL(a)) {
    return b;
  } else if (IS_BOOL(a)) {
    if (IS_NIL(b) || (IS_BOOL(b) && AS_BOOL(b) == false))
      return a; // only nil, false and false are lower than numbers
    else
      return b;
  } else if (IS_NUMBER(a)) {
    if (IS_NIL(b) || IS_BOOL(b))
      return a;
    else if (IS_NUMBER(b))
      return AS_NUMBER(a) >= AS_NUMBER(b) ? a : b;
    else
      return b; // every other thing is greater than a number
  } else if (IS_OBJ(a)) {
    if (IS_STRING(a) && IS_STRING(b)) {
      return strcmp(AS_C_STRING(a), AS_C_STRING(b)) >= 0 ? a : b;
    } else if (IS_FUNCTION(a) && IS_FUNCTION(b)) {
      return AS_FUNCTION(a)->arity >= AS_FUNCTION(b)->arity
             ? a
             : b;
    } else if (IS_CLOSURE(a) && IS_CLOSURE(b)) {
      return AS_CLOSURE(a)->function->arity >= AS_CLOSURE(b)->function->arity
             ? a
             : b;
    } else if (IS_RANGE(a) && IS_RANGE(b)) {
      return AS_RANGE(a)->lower >= AS_RANGE(b)->lower ? a : b;
    } else if (IS_CLASS(a) && IS_CLASS(b)) {
      return AS_CLASS(a)->methods.count >= AS_CLASS(b)->methods.count ? a : b;
    } else if (IS_LIST(a) && IS_LIST(b)) {
      return AS_LIST(a)->items.count >= AS_LIST(b)->items.count ? a : b;
    } else if (IS_DICT(a) && IS_DICT(b)) {
      return AS_DICT(a)->names.count >= AS_DICT(b)->names.count ? a : b;
    } else if (IS_BYTES(a) && IS_BYTES(b)) {
      return AS_BYTES(a)->bytes.count >= AS_BYTES(b)->bytes.count ? a : b;
    } else if (IS_FILE(a) && IS_FILE(b)) {
      return strcmp(AS_FILE(a)->path->chars, AS_FILE(b)->path->chars) >= 0 ? a : b;
    } else if (IS_OBJ(b)) {
      return AS_OBJ(a)->type >= AS_OBJ(b)->type ? a : b;
    } else {
      return a;
    }
  } else {
    return a;
  }
}

/**
 * sorts values in an array using the bubble-sort algorithm
 */
void sort_values(b_value *values, int count) {
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      if (values_equal(values[j], find_max_value(values[i], values[j]))) {
        b_value temp = values[i];
        values[i] = values[j];
        values[j] = temp;

        if (IS_LIST(values[i]))
          sort_values(AS_LIST(values[i])->items.values,
                      AS_LIST(values[i])->items.count);

        if (IS_LIST(values[j]))
          sort_values(AS_LIST(values[j])->items.values,
                      AS_LIST(values[j])->items.count);
      }
    }
  }
}
