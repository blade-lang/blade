#include "object.h"
#include "config.h"
#include "memory.h"
#include "table.h"
#include "util.h"
#include "value.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_OBJ(type, obj_type)                                           \
  (type *)allocate_object(vm, sizeof(type), obj_type)

static b_obj *allocate_object(b_vm *vm, size_t size, b_obj_type type) {
  b_obj *object = (b_obj *)reallocate(vm, NULL, 0, size);

  object->type = type;
  object->is_marked = false;

  object->next = vm->objects;
  vm->objects = object;

#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC == 1
  printf("%p allocate %ld for %d\n", (void *)object, size, type);
#endif

  return object;
}

b_obj_bytes *new_bytes(b_vm *vm, int length) {
  b_obj_bytes *bytes = ALLOCATE_OBJ(b_obj_bytes, OBJ_BYTES);
  init_byte_arr(&bytes->bytes, length);

  bytes->bytes.bytes =
      reallocate(vm, bytes->bytes.bytes, 0, sizeof(unsigned char *) * length);
  bytes->bytes.count = length;

  return bytes;
}

b_obj_list *new_list(b_vm *vm) {
  b_obj_list *list = ALLOCATE_OBJ(b_obj_list, OBJ_LIST);
  init_value_arr(&list->items);
  return list;
}

b_obj_dict *new_dict(b_vm *vm) {
  b_obj_dict *dict = ALLOCATE_OBJ(b_obj_dict, OBJ_DICT);
  init_value_arr(&dict->names);
  init_table(&dict->items);
  return dict;
}

b_obj_file *new_file(b_vm *vm, b_obj_string *path, b_obj_string *mode) {
  b_obj_file *file = ALLOCATE_OBJ(b_obj_file, OBJ_FILE);
  file->is_open = true;
  file->mode = mode;
  file->path = path;
  file->file = NULL;
  return file;
}

b_obj_bound *new_bound_method(b_vm *vm, b_value receiver, b_obj *method) {
  b_obj_bound *bound = ALLOCATE_OBJ(b_obj_bound, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

b_obj_class *new_class(b_vm *vm, b_obj_string *name) {
  b_obj_class *klass = ALLOCATE_OBJ(b_obj_class, OBJ_CLASS);
  klass->name = name;
  init_table(&klass->fields);
  init_table(&klass->static_fields);
  init_table(&klass->methods);
  init_table(&klass->static_methods);
  klass->initializer = EMPTY_VAL;
  klass->superclass = NULL;
  return klass;
}

b_obj_func *new_function(b_vm *vm) {
  b_obj_func *function = ALLOCATE_OBJ(b_obj_func, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->is_variadic = false;
  function->name = NULL;
  init_blob(&function->blob);
  return function;
}

b_obj_instance *new_instance(b_vm *vm, b_obj_class *klass) {
  b_obj_instance *instance = ALLOCATE_OBJ(b_obj_instance, OBJ_INSTANCE);
  instance->klass = klass;
  init_table(&instance->fields);
  table_add_all(vm, &klass->fields, &instance->fields);
  return instance;
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
  string->utf8_length = utf8len(chars);
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
    printf("<script at %p>", (void *)function);
  } else {
    printf("<function %s at %p>", function->name->chars, (void *)function);
  }
}

static void print_list(b_obj_list *list) {
  printf("[");
  for (int i = 0; i < list->items.count; i++) {
    print_value(list->items.values[i]);
    if (i != list->items.count - 1) {
      printf(", ");
    }
  }
  printf("]");
}

static void print_bytes(b_obj_bytes *bytes) {
  printf("(");
  for (int i = 0; i < bytes->bytes.count; i++) {
    printf("0x%x", bytes->bytes.bytes[i]);
    if (i > 10) {
      printf("...");
      break;
    }

    if (i != bytes->bytes.count - 1) {
      printf(" ");
    }
  }
  printf(")");
}

static void print_dict(b_obj_dict *dict) {
  printf("{");
  for (int i = 0; i < dict->names.count; i++) {
    print_value(dict->names.values[i]);

    printf(": ");

    b_value value;
    table_get(&dict->items, dict->names.values[i], &value);
    print_value(value);

    if (i != dict->names.count - 1) {
      printf(", ");
    }
  }
  printf("}");
}

static void print_file(b_obj_file *file) {
  printf("<file at %s in mode %s>", file->path->chars, file->mode->chars);
}

void print_object(b_value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_FILE: {
    print_file(AS_FILE(value));
    break;
  }
  case OBJ_DICT: {
    print_dict(AS_DICT(value));
    break;
  }
  case OBJ_LIST: {
    print_list(AS_LIST(value));
    break;
  }
  case OBJ_BYTES: {
    print_bytes(AS_BYTES(value));
    break;
  }

  case OBJ_BOUND_METHOD: {
    b_obj *method = AS_BOUND(value)->method;
    if (method->type == OBJ_CLOSURE) {
      print_function(((b_obj_closure *)method)->function);
    } else {
      print_function((b_obj_func *)method);
    }
    break;
  }
  case OBJ_CLASS: {
    printf("<class %s at %p>", AS_CLASS(value)->name->chars,
           (void *)AS_CLASS(value));
    break;
  }
  case OBJ_CLOSURE: {
    print_function(AS_CLOSURE(value)->function);
    break;
  }
  case OBJ_FUNCTION: {
    print_function(AS_FUNCTION(value));
    break;
  }
  case OBJ_INSTANCE: {
    // @TODO: support the to_string() override
    b_obj_instance *instance = AS_INSTANCE(value);
    printf("<class %s instance at %p>", instance->klass->name->chars,
           (void *)instance);
    break;
  }
  case OBJ_NATIVE: {
    b_obj_native *native = AS_NATIVE(value);
    printf("<function(native) %s at %p>", native->name, (void *)native);
    break;
  }
  case OBJ_UPVALUE: {
    printf("upvalue");
    break;
  }
  case OBJ_STRING: {
    printf("%s", AS_CSTRING(value));
    break;
  }
  }
}

b_obj_bytes *copy_bytes(b_vm *vm, unsigned char *b, int length) {
  b_obj_bytes *bytes = new_bytes(vm, length);

  memcpy(bytes->bytes.bytes, b, length * sizeof(unsigned char *));
  return bytes;
}

b_obj_bytes *take_bytes(b_vm *vm, unsigned char *b, int length) {
  b_obj_bytes *bytes = new_bytes(vm, length);

  bytes->bytes.bytes = b;
  return bytes;
}

static inline char *function_to_string(b_obj_func *func) {
  if (func->name == NULL) {
    return "<script 0x00>";
  }
  char *str = (char *)calloc(1, sizeof(char *));
  sprintf(str, "<function %s>", func->name->chars);
  return str;
}

static inline char *list_to_string(b_vm *vm, b_value_arr *array) {
  char *str = "[";
  for (int i = 0; i < array->count; i++) {
    str = append_strings(str, value_to_string(vm, array->values[i]));
    if (i != array->count - 1) {
      str = append_strings(str, ", ");
    }
  }
  str = append_strings(str, "]");
  return str;
}

static inline char *bytes_to_string(b_vm *vm, b_byte_arr *array) {
  char *str = "(";
  for (int i = 0; i < array->count; i++) {
    char *chrs = (char *)calloc(1, sizeof(char *));
    sprintf(chrs, "0x%x", array->bytes[i]);

    if (i != array->count - 1) {
      str = append_strings(str, " ");
    }
  }
  str = append_strings(str, ")");
  return str;
}

static char *dict_to_string(b_vm *vm, b_obj_dict *dict) {
  char *str = "{";
  for (int i = 0; i < dict->names.count; i++) {
    // print_value(dict->names.values[i]);
    b_value key = dict->names.values[i];
    str = append_strings(str, value_to_string(vm, key));
    str = append_strings(str, ": ");

    b_value value;
    table_get(&dict->items, key, &value);
    str = append_strings(str, value_to_string(vm, value));

    if (i != dict->names.count - 1) {
      str = append_strings(str, ", ");
    }
  }
  str = append_strings(str, "}");
  return str;
}

char *object_to_string(b_vm *vm, b_value value) {
  char *str = (char *)calloc(1, sizeof(char *));

  switch (OBJ_TYPE(value)) {
  case OBJ_CLASS:
    sprintf(str, "<class %s>", AS_CLASS(value)->name->chars);
    break;
  case OBJ_INSTANCE:
    sprintf(str, "<instance of %s>", AS_INSTANCE(value)->klass->name->chars);
    break;
  case OBJ_CLOSURE:
    return function_to_string(AS_CLOSURE(value)->function);
  case OBJ_BOUND_METHOD: {
    b_obj *method = AS_BOUND(value)->method;
    if (method->type == OBJ_CLOSURE) {
      return function_to_string(((b_obj_closure *)method)->function);
    }
    return function_to_string((b_obj_func *)method);
  }
  case OBJ_FUNCTION:
    return function_to_string(AS_FUNCTION(value));
  case OBJ_NATIVE:
    sprintf(str, "<native-function %s>", AS_NATIVE(value)->name);
    break;
  case OBJ_STRING:
    return AS_CSTRING(value);
  case OBJ_UPVALUE:
    return (char *)"<upvalue>";
  case OBJ_BYTES:
    return bytes_to_string(vm, &AS_BYTES(value)->bytes);
  case OBJ_LIST:
    return list_to_string(vm, &AS_LIST(value)->items);
  case OBJ_DICT:
    return dict_to_string(vm, AS_DICT(value));
  case OBJ_FILE: {
    b_obj_file *file = AS_FILE(value);
    sprintf(str, "<file at %s in mode %s>", file->path->chars,
            file->mode->chars);
    break;
  }
  }

  return str;
}

const char *object_type(b_obj *object) {
  switch (object->type) {
  case OBJ_BYTES:
    return "bytes";
  case OBJ_FILE:
    return "file";
  case OBJ_DICT:
    return "dictionary";
  case OBJ_LIST:
    return "list";

  case OBJ_CLASS:
    return "class";

  case OBJ_FUNCTION:
  case OBJ_NATIVE:
  case OBJ_CLOSURE:
  case OBJ_BOUND_METHOD:
    return "function";

  case OBJ_INSTANCE:
    return ((b_obj_instance *)object)->klass->name->chars;

  case OBJ_STRING:
    return "string";

  default:
    return "unknown";
  }
}