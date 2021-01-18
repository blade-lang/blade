#include <stdio.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type)                                           \
  (type *)allocate_object(vm, sizeof(type), obj_type)

static b_obj *allocate_object(b_vm *vm, size_t size, b_obj_type type) {
  b_obj *object = (b_obj *)reallocate(vm, NULL, 0, size);

  object->type = type;
  object->is_marked = false;

  object->next = vm->objects;
  vm->objects = object;

#if DEBUG_MODE == 1
#if DEBUG_LOG_GC == 1
  printf("%p allocate %ld for %d\n", (void *)object, size, type);
#endif
#endif

  return object;
}

b_obj_func *new_function(b_vm *vm) {
  b_obj_func *function = ALLOCATE_OBJ(b_obj_func, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->name = NULL;
  init_blob(&function->blob);
  return function;
}

b_obj_native *new_native(b_vm *vm, b_native_fn function, const char *name) {
  b_obj_native *native = ALLOCATE_OBJ(b_obj_native, OBJ_NATIVE);
  native->function = function;
  native->name = name;
  return native;
}

b_obj_closure *new_closure(b_vm *vm, b_obj_func *function) {
  b_obj_upvalue **upvalues = ALLOCATE(b_obj_upvalue *, function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }

  b_obj_closure *closure = ALLOCATE_OBJ(b_obj_closure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

b_obj_string *allocate_string(b_vm *vm, char *chars, int length,
                              uint32_t hash) {
  b_obj_string *string = ALLOCATE_OBJ(b_obj_string, OBJ_STRING);
  string->chars = chars;
  string->length = length;
  string->hash = hash;

  push(vm, OBJ_VAL(string)); // fixing gc corruption
  table_set(vm, &vm->strings, OBJ_VAL(string), NIL_VAL);
  pop(vm); // fixing gc corruption

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

b_obj_upvalue *new_upvalue(b_vm *vm, b_value *slot) {
  b_obj_upvalue *upvalue = ALLOCATE_OBJ(b_obj_upvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void print_function(b_obj_func *function) {
  if (function->name == NULL) {
    printf("<script at 0x%lx>", (long)function);
  } else {
    printf("<function %s at 0x%lx>", function->name->chars, (long)function);
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
  case OBJ_CLOSURE: {
    print_function(AS_CLOSURE(value)->function);
    break;
  }
  case OBJ_NATIVE: {
    b_obj_native *native = AS_NATIVE(value);
    printf("<function(native) %s at 0x%lx>", native->name, (long)native);
    break;
  }
  case OBJ_UPVALUE: {
    printf("upvalue");
    break;
  }
  }
}

const char *object_type(b_obj *object) {
  switch (object->type) {
  case OBJ_STRING:
    return "string";

  case OBJ_FUNCTION:
  case OBJ_NATIVE:
  case OBJ_CLOSURE:
    return "function";

  default:
    return "unknown";
  }
}