#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type)                                           \
  (type *)allocate_object(vm, sizeof(type), obj_type)

static b_obj *allocate_object(b_vm *vm, size_t size, b_obj_type type) {
  b_obj *object = (b_obj *)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm->objects;
  vm->objects = object;

  return object;
}

b_obj_func *new_function(b_vm *vm) {
  b_obj_func *function = ALLOCATE_OBJ(b_obj_func, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  init_blob(&function->blob);
  return function;
}

b_obj_string *allocate_string(b_vm *vm, char *chars, int length,
                              uint32_t hash) {
  b_obj_string *string = ALLOCATE_OBJ(b_obj_string, OBJ_STRING);
  string->chars = chars;
  string->length = length;
  string->hash = hash;

  table_set(&vm->strings, OBJ_VAL(string), NIL_VAL);

  return string;
}

b_obj_string *take_string(b_vm *vm, char *chars, int length) {
  uint32_t hash = hash_string(chars, length);

  b_obj_string *interned = table_find_string(&vm->strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocate_string(vm, chars, length, hash);
}

b_obj_string *copy_string(b_vm *vm, const char *chars, int length) {
  uint32_t hash = hash_string(chars, length);

  b_obj_string *interned = table_find_string(&vm->strings, chars, length, hash);
  if (interned != NULL)
    return interned;

  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return allocate_string(vm, heap_chars, length, hash);
}

static void print_function(b_obj_func *function) {
  if (function->name == NULL) {
    printf("<script>");
  } else {
    printf("<fn %s>", function->name->chars);
  }
}

void print_object(b_value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING: {
    printf("%s", AS_CSTRING(value));
    break;
  }
  case OBJ_FUNCTION: {
    print_function(AS_FUNCTION(value));
    break;
  }
  }
}

const char *object_type(b_obj *object) {
  switch (object->type) {
  case OBJ_STRING:
    return "string";
  case OBJ_FUNCTION:
    return "function";

  default:
    return "unknown";
  }
}