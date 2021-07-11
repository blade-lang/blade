#include "memory.h"
#include "compiler.h"
#include "config.h"
#include "object.h"
#include "b_file.h"

#include <stdio.h>
#include <stdlib.h>

#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

void *reallocate(b_vm *vm, void *pointer, size_t old_size, size_t new_size) {
  vm->bytes_allocated += new_size - old_size;

  if (new_size > old_size && vm->bytes_allocated > vm->next_gc) {
    collect_garbage(vm);
  }

  if (new_size == 0) {
    free(pointer);
   return NULL;
    // return malloc(sizeof pointer);
  }
  void *result = realloc(pointer, new_size);

  // just in case reallocation fails... computers ain't infinite!
  if (result == NULL) {
    fflush(stdout); // flush out anything on stdout first
    fprintf(stderr, "Exit: device out of memory\n");
    exit(EXIT_TERMINAL);
  }
  return result;
}

void mark_object(b_vm *vm, b_obj *object) {
  if (object == NULL)
    return;
  if (object->mark == vm->mark_value)
    return;

#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
  printf("%p mark ", (void *)object);
  print_object(OBJ_VAL(object), false);
  printf("\n");
#endif

  object->mark = vm->mark_value;

  if (vm->gray_capacity < vm->gray_count + 1) {
    vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
    b_obj **result =
        (b_obj **)realloc(vm->gray_stack, sizeof(b_obj *) * vm->gray_capacity);

    if (result == NULL) {
      fflush(stdout); // flush out anything on stdout first
      fprintf(stderr, "GC encountered an error");
      exit(1);
    }

    vm->gray_stack = result;
  }
  vm->gray_stack[vm->gray_count++] = object;
}

void mark_value(b_vm *vm, b_value value) {
  if (IS_OBJ(value))
    mark_object(vm, AS_OBJ(value));
}

static void mark_array(b_vm *vm, b_value_arr *array) {
  for (int i = 0; i < array->count; i++) {
    mark_value(vm, array->values[i]);
  }
}

static void blacken_object(b_vm *vm, b_obj *object) {
#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
  printf("%p blacken ", (void *)object);
  print_object(OBJ_VAL(object), false);
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_SWITCH: {
      b_obj_switch *sw = (b_obj_switch *)object;
      mark_table(vm, &sw->table);
      break;
    }
    case OBJ_FILE: {
      b_obj_file *file = (b_obj_file *)object;
      mark_object(vm, (b_obj *)file->mode);
      mark_object(vm, (b_obj *)file->path);
      break;
    }
    case OBJ_DICT: {
      b_obj_dict *dict = (b_obj_dict *)object;
      mark_array(vm, &dict->names);
      mark_table(vm, &dict->items);
      break;
    }
    case OBJ_LIST: {
      b_obj_list *list = (b_obj_list *)object;
      mark_array(vm, &list->items);
      break;
    }

    case OBJ_BOUND_METHOD: {
      b_obj_bound *bound = (b_obj_bound *)object;
      mark_value(vm, bound->receiver);
      mark_object(vm, bound->method);
      break;
    }
    case OBJ_CLASS: {
      b_obj_class *klass = (b_obj_class *)object;
      mark_object(vm, (b_obj *)klass->name);
      mark_table(vm, &klass->methods);
      mark_table(vm, &klass->properties);
      mark_table(vm, &klass->static_properties);
      break;
    }
    case OBJ_CLOSURE: {
      b_obj_closure *closure = (b_obj_closure *)object;
      mark_object(vm, (b_obj *)closure->function);
      for (int i = 0; i < closure->up_value_count; i++) {
        mark_object(vm, (b_obj *)closure->up_values[i]);
      }
      break;
    }

    case OBJ_FUNCTION: {
      b_obj_func *function = (b_obj_func *)object;
      mark_object(vm, (b_obj *)function->name);
      mark_array(vm, &function->blob.constants);
      break;
    }
    case OBJ_INSTANCE: {
      b_obj_instance *instance = (b_obj_instance *)object;
      mark_object(vm, (b_obj *)instance->klass);
      mark_table(vm, &instance->properties);
      break;
    }

    case OBJ_UP_VALUE: {
      mark_value(vm, ((b_obj_up_value *)object)->closed);
      break;
    }

    case OBJ_BYTES: {
      mark_object(vm, object);
      break;
    }

    case OBJ_NATIVE: {
      mark_object(vm, object);
      break;
    }
    case OBJ_STRING:
      break;
  }
}

static void free_object(b_vm *vm, b_obj *object) {
#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
  printf("%p free type %d\n", (void *)object, object->type);
#endif

  switch (object->type) {
    case OBJ_SWITCH: {
      b_obj_switch *sw = (b_obj_switch *)object;
      free_table(vm, &sw->table);
      FREE(b_obj_switch, object);
      break;
    }
    case OBJ_BYTES: {
      b_obj_bytes *bytes = (b_obj_bytes *)object;
      free_byte_arr(vm, &bytes->bytes);
      FREE(b_obj_bytes, object);
      break;
    }
    case OBJ_FILE: {
      b_obj_file *file = (b_obj_file *)object;
      if (file->mode->length != 0 && !is_std_file(file)) {
        fclose(file->file);
      }
      FREE(b_obj_file, object);
      break;
    }
    case OBJ_DICT: {
      b_obj_dict *dict = (b_obj_dict *)object;
      free_value_arr(vm, &dict->names);
      free_table(vm, &dict->items);
      FREE(b_obj_dict, object);
      break;
    }
    case OBJ_LIST: {
      b_obj_list *list = (b_obj_list *)object;
      free_value_arr(vm, &list->items);
      FREE(b_obj_list, object);
      break;
    }

    case OBJ_BOUND_METHOD: {
      // a closure may be bound to multiple instances
      // for this reason, we do not free closures when freeing bound methods
      FREE(b_obj_bound, object);
      break;
    }
    case OBJ_CLASS: {
      b_obj_class *klass = (b_obj_class *)object;
      free_object(vm, (b_obj *)klass->name);
      free_table(vm, &klass->methods);
      free_table(vm, &klass->properties);
      free_table(vm, &klass->static_properties);
      FREE(b_obj_class, object);
      break;
    }
    case OBJ_CLOSURE: {
      b_obj_closure *closure = (b_obj_closure *)object;
      FREE_ARRAY(b_obj_up_value *, closure->up_values, closure->up_value_count);
      // there may be multiple closures that all reference the same function
      // for this reason, we do not free functions when freeing closures
      FREE(b_obj_closure, object);
      break;
    }
    case OBJ_FUNCTION: {
      b_obj_func *function = (b_obj_func *)object;
      free_blob(vm, &function->blob);
      free_object(vm, (b_obj *)function->name);
      function->file = NULL;
      FREE(b_obj_func, object);
      break;
    }
    case OBJ_INSTANCE: {
      b_obj_instance *instance = (b_obj_instance *)object;
      free_table(vm, &instance->properties);
      FREE(b_obj_instance, object);
      break;
    }
    case OBJ_NATIVE: {
      FREE(b_obj_native, object);
      break;
    }
    case OBJ_UP_VALUE: {
      FREE(b_obj_up_value, object);
      break;
    }
    case OBJ_STRING: {
      b_obj_string *string = (b_obj_string *)object;
      FREE_ARRAY(char, string->chars, (size_t)string->length + 1);
      FREE(b_obj_string, object);
      break;
    }

    default:
      break;
  }
}

static void mark_roots(b_vm *vm) {
  for (b_value *slot = vm->stack; slot < vm->stack_top; slot++) {
    mark_value(vm, *slot);
  }
  for (int i = 0; i < vm->frame_count; i++) {
    mark_object(vm, (b_obj *)vm->frames[i].function);
  }
  for (b_obj_up_value *up_value = vm->open_up_values; up_value != NULL;
       up_value = up_value->next) {
    mark_object(vm, (b_obj *)up_value);
  }
  mark_table(vm, &vm->globals);

  mark_table(vm, &vm->methods_string);
  mark_table(vm, &vm->methods_bytes);
  mark_table(vm, &vm->methods_file);
  mark_table(vm, &vm->methods_list);
  mark_table(vm, &vm->methods_dict);

  mark_compiler_roots(vm);
}

static void trace_references(b_vm *vm) {
  while (vm->gray_count > 0) {
    b_obj *object = vm->gray_stack[--vm->gray_count];
    blacken_object(vm, object);
  }
}

static void sweep(b_vm *vm) {
  b_obj *previous = NULL;
  b_obj *object = vm->objects;

  while (object != NULL) {
    if (object->mark == vm->mark_value) {
      previous = object;
      object = object->next;
    } else {
      b_obj *unreached = object;

      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm->objects = object;
      }

      free_object(vm, unreached);
    }
  }
}

void free_objects(b_vm *vm) {
  b_obj *object = vm->objects;
  while (object != NULL) {
    b_obj *next = object->next;
    free_object(vm, object);
    object = next;
  }

  free(vm->gray_stack);
  vm->gray_stack = NULL;
}

void collect_garbage(b_vm *vm) {
#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
  printf("-- gc begins\n");
  size_t before = vm->bytes_allocated;
#endif

  mark_roots(vm);
  trace_references(vm);
  table_remove_whites(vm, &vm->strings);
  sweep(vm);

  vm->next_gc = vm->bytes_allocated * GC_HEAP_GROWTH_FACTOR;
  vm->mark_value = !vm->mark_value;

#if defined DEBUG_LOG_GC && DEBUG_LOG_GC
  printf("-- gc ends\n");
  printf("   collected %zu bytes (from %zu to %zu), next at %zu\n",
         before - vm->bytes_allocated, before, vm->bytes_allocated,
         vm->next_gc);
#endif
}