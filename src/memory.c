#include <stdlib.h>

#include "memory.h"
#include "object.h"

void *reallocate(void *pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }
  void *result = realloc(pointer, new_size);

  // just in case reallocation fails... computers aint infinite!
  if (result == NULL) {
    // @TODO tell the user exactly why we are exiting...
    // @WaitReason Not yet decided...
    // some message like: Sorry, your computer ran out of memeory
    exit(1);
  }
  return result;
}

static void free_object(b_obj *object) {

  switch (object->type) {
  case OBJ_STRING: {
    b_obj_string *string = (b_obj_string *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(b_obj_string, string);
    break;
  }
  case OBJ_FUNCTION: {
    b_obj_func *function = (b_obj_func *)object;
    free_blob(&function->blob);
    FREE(b_obj_func, function);
    break;
  }
  case OBJ_NATIVE: {
    FREE(b_obj_native, object);
    break;
  }
  case OBJ_CLOSURE: {
    b_obj_closure *closure = (b_obj_closure *)object;
    FREE_ARRAY(b_obj_upvalue *, closure->upvalues, closure->upvalue_count);
    // there may be multiple closures that all reference the same function
    // for this reason, we do not free functions when freeing closures
    FREE(b_obj_closure, object);
    break;
  }
  case OBJ_UPVALUE: {
    FREE(b_obj_upvalue, object);
    break;
  }

  default:
    break;
  }
}

void free_objects(b_vm *vm) {
  b_obj *object = vm->objects;
  while (object != NULL) {
    b_obj *next = object->next;
    free_object(object);
    object = next;
  }
}