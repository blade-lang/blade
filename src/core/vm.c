#include "vm.h"
#include "common.h"
#include "compat/asprintf.h"
#include "compiler.h"
#include "config.h"
#include "memory.h"
#include "module.h"
#include "native.h"
#include "object.h"

#include "builtin/bytes.h"
#include "builtin/dict.h"
#include "builtin/file.h"
#include "builtin/list.h"
#include "builtin/string.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#if defined DEBUG_MODE && DEBUG_MODE
#include "debug.h"
#endif

static void reset_stack(b_vm *vm) {
  vm->stack_top = vm->stack;
  vm->frame_count = 0;
  vm->open_up_values = NULL;
  vm->catch_frame = NULL;
}

static inline b_obj_func *get_frame_function(b_call_frame *frame) {
  if (frame->function->type == OBJ_FUNCTION) {
    return (b_obj_func *)frame->function;
  } else {
    return ((b_obj_closure *)frame->function)->function;
  }
}

DECLARE_NATIVE(__exception_trace__) {
  char *trace = (char *)malloc(sizeof(char));
  memset(trace, 0, sizeof(char));

  for (int i = 0; i < vm->frame_count; i++) {
    b_call_frame *frame = &vm->frames[i];
    b_obj_func *function = get_frame_function(frame);

    // -1 because the IP is sitting on the next instruction to be executed
    size_t instruction = frame->ip - get_frame_function(frame)->blob.code - 1;

    char *trace_part = NULL;
    asprintf(&trace_part,
             "    File: %s, Line: %d, In: ", get_frame_function(frame)->file,
             function->blob.lines[instruction]);

    if (function->name == NULL) {
      trace_part = append_strings(
          trace_part, i < vm->frame_count - 1 ? "<script>\n" : "<script>");
    } else {
      trace_part = append_strings(trace_part, function->name->chars);
      trace_part =
          append_strings(trace_part, i < vm->frame_count - 1 ? "()\n" : "()");
    }

    trace = append_strings(trace, trace_part);
  }

  RETURN_STRING(trace);
}

static void initialize_exceptions(b_vm *vm) {
  b_obj_string *class_name = copy_string(vm, "Exception", 9);
  b_obj_class *klass = new_class(vm, class_name);

  b_value initializer =
      OBJ_VAL(new_native(vm, GET_NATIVE(__Exception__), class_name->chars));

  // set class constructor
  table_set(vm, &klass->methods,
            STRING_L_VAL(class_name->chars, class_name->length), initializer);
  klass->initializer = initializer;

  // set class fields
  table_set(vm, &klass->fields, STRING_L_VAL("message", 7), NIL_VAL);
  b_obj_native *trace_fn =
      new_native(vm, GET_NATIVE(__exception_trace__), "trace");
  table_set(vm, &klass->methods, STRING_L_VAL("trace", 5), OBJ_VAL(trace_fn));

  table_set(vm, &vm->globals, OBJ_VAL(class_name), OBJ_VAL(klass));
  vm->exception_class = klass;
}

b_obj_instance *create_exception(b_vm *vm, b_obj_string *message) {
  char *trace = (char *)malloc(sizeof(char));
  memset(trace, 0, sizeof(char));

  //  // fprintf(stderr, "StackTrace:\n");
  //  for (int i = 0; i < vm->frame_count; i++) {
  //    b_call_frame *frame = &vm->frames[i];
  //    b_obj_func *function = get_frame_function(frame);
  //
  //    // -1 because the IP is sitting on the next instruction to be executed
  //    size_t instruction = frame->ip - get_frame_function(frame)->blob.code -
  //    1;
  //
  //    char *trace_part = NULL;
  //    asprintf(&trace_part,
  //             "    File: %s, Line: %d, In: ",
  //             get_frame_function(frame)->file,
  //             function->blob.lines[instruction]);
  //
  //    if (function->name == NULL) {
  //      trace_part = append_strings(
  //          trace_part, i < vm->frame_count - 1 ? "<script>\n" : "<script>");
  //    } else {
  //      trace_part = append_strings(trace_part, function->name->chars);
  //      trace_part =
  //          append_strings(trace_part, i < vm->frame_count - 1 ? "()\n" :
  //          "()");
  //    }
  //
  //    trace = append_strings(trace, trace_part);
  //  }

  b_obj_instance *instance = new_instance(vm, vm->exception_class);
  table_set(vm, &instance->fields, STRING_L_VAL("message", 7), OBJ_VAL(message));
  //  table_set(vm, &instance->fields, OBJ_VAL(copy_string(vm, "trace", 5)),
  //            OBJ_VAL(take_string(vm, trace, (int)strlen(trace))));
  return instance;
}

void _runtime_error(b_vm *vm, const char *format, ...) {

  // only throw error when there is no surrounding try...catch... statement
  if (vm->catch_frame == NULL) {

    b_call_frame *frame = &vm->frames[vm->frame_count - 1];

    size_t instruction = frame->ip - get_frame_function(frame)->blob.code - 1;
    int line = get_frame_function(frame)->blob.lines[instruction];

    fprintf(stderr, "RuntimeError:\n");
    fprintf(stderr, "    File: %s, Line: %d\n    Message: ",
            get_frame_function(frame)->file, line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    if (vm->frame_count > 1) {
      fprintf(stderr, "StackTrace:\n");
      for (int i = vm->frame_count - 1; i >= 0; i--) {
        frame = &vm->frames[i];
        b_obj_func *function = get_frame_function(frame);

        // -1 because the IP is sitting on the next instruction to be executed
        instruction = frame->ip - get_frame_function(frame)->blob.code - 1;

        fprintf(stderr,
                "    File: %s, Line: %d, In: ", get_frame_function(frame)->file,
                function->blob.lines[instruction]);
        if (function->name == NULL) {
          fprintf(stderr, "<script>\n");
        } else {
          fprintf(stderr, "%s()\n", function->name->chars);
        }
      }
    }

    reset_stack(vm);
  } else {
    char *message = NULL;

    va_list args;
    va_start(args, format);
    int length = vasprintf(&message, format, args);
    va_end(args);

    push(vm, OBJ_VAL(create_exception(vm, take_string(vm, message, length))));
  }
}

void push(b_vm *vm, b_value value) {
  *vm->stack_top = value;
  vm->stack_top++;
}

b_value pop(b_vm *vm) {
  vm->stack_top--;
  return *vm->stack_top;
}

b_value pop_n(b_vm *vm, int n) {
  vm->stack_top -= n;
  return *vm->stack_top;
}

b_value peek(b_vm *vm, int distance) { return vm->stack_top[-1 - distance]; }

static void define_native(b_vm *vm, const char *name, b_native_fn function) {
  push(vm, STRING_VAL(name));
  push(vm, OBJ_VAL(new_native(vm, function, name)));
  table_set(vm, &vm->globals, vm->stack[0], vm->stack[1]);
  pop_n(vm, 2);
}

void define_native_method(b_vm *vm, b_table *table, const char *name,
                          b_native_fn function) {
  push(vm, STRING_VAL(name));
  push(vm, OBJ_VAL(new_native(vm, function, name)));
  table_set(vm, table, vm->stack[0], vm->stack[1]);
  pop_n(vm, 2);
}

static void init_builtin_functions(b_vm *vm) {
  DEFINE_NATIVE(abs);
  DEFINE_NATIVE(bin);
  DEFINE_NATIVE(bytes);
  DEFINE_NATIVE(chr);
  DEFINE_NATIVE(delprop);
  DEFINE_NATIVE(file);
  DEFINE_NATIVE(getprop);
  DEFINE_NATIVE(hash);
  DEFINE_NATIVE(hasprop);
  DEFINE_NATIVE(hex);
  DEFINE_NATIVE(id);
  DEFINE_NATIVE(int);
  DEFINE_NATIVE(is_bool);
  DEFINE_NATIVE(is_callable);
  DEFINE_NATIVE(is_class);
  DEFINE_NATIVE(is_dict);
  DEFINE_NATIVE(is_function);
  DEFINE_NATIVE(is_instance);
  DEFINE_NATIVE(is_int);
  DEFINE_NATIVE(is_list);
  DEFINE_NATIVE(is_number);
  DEFINE_NATIVE(is_object);
  DEFINE_NATIVE(is_string);
  DEFINE_NATIVE(is_file);
  DEFINE_NATIVE(is_iterable);
  DEFINE_NATIVE(max);
  DEFINE_NATIVE(microtime);
  DEFINE_NATIVE(min);
  DEFINE_NATIVE(oct);
  DEFINE_NATIVE(ord);
  DEFINE_NATIVE(print);
  DEFINE_NATIVE(rand);
  DEFINE_NATIVE(setprop);
  DEFINE_NATIVE(sum);
  DEFINE_NATIVE(time);
  DEFINE_NATIVE(to_bool);
  DEFINE_NATIVE(to_dict);
  DEFINE_NATIVE(to_int);
  DEFINE_NATIVE(to_list);
  DEFINE_NATIVE(to_number);
  DEFINE_NATIVE(to_string);
  DEFINE_NATIVE(typeof);
}

static void init_builtin_methods(b_vm *vm) {
#define DEFINE_STRING_METHOD(name) DEFINE_METHOD(string, name)
#define DEFINE_LIST_METHOD(name) DEFINE_METHOD(list, name)
#define DEFINE_DICT_METHOD(name) DEFINE_METHOD(dict, name)
#define DEFINE_FILE_METHOD(name) DEFINE_METHOD(file, name)
#define DEFINE_BYTES_METHOD(name) DEFINE_METHOD(bytes, name)

  // string methods
  DEFINE_STRING_METHOD(length);
  DEFINE_STRING_METHOD(upper);
  DEFINE_STRING_METHOD(lower);
  DEFINE_STRING_METHOD(is_alpha);
  DEFINE_STRING_METHOD(is_alnum);
  DEFINE_STRING_METHOD(is_number);
  DEFINE_STRING_METHOD(is_lower);
  DEFINE_STRING_METHOD(is_upper);
  DEFINE_STRING_METHOD(is_space);
  DEFINE_STRING_METHOD(trim);
  DEFINE_STRING_METHOD(ltrim);
  DEFINE_STRING_METHOD(rtrim);
  DEFINE_STRING_METHOD(join);
  DEFINE_STRING_METHOD(split);
  DEFINE_STRING_METHOD(index_of);
  DEFINE_STRING_METHOD(starts_with);
  DEFINE_STRING_METHOD(ends_with);
  DEFINE_STRING_METHOD(count);
  DEFINE_STRING_METHOD(to_number);
  DEFINE_STRING_METHOD(to_list);
  DEFINE_STRING_METHOD(to_bytes);
  DEFINE_STRING_METHOD(lpad);
  DEFINE_STRING_METHOD(rpad);
  DEFINE_STRING_METHOD(match);
  DEFINE_STRING_METHOD(matches);
  DEFINE_STRING_METHOD(replace);
  DEFINE_STRING_METHOD(__iter__);
  DEFINE_STRING_METHOD(__itern__);

  // list methods
  DEFINE_LIST_METHOD(length);
  DEFINE_LIST_METHOD(append);
  DEFINE_LIST_METHOD(clear);
  DEFINE_LIST_METHOD(clone);
  DEFINE_LIST_METHOD(count);
  DEFINE_LIST_METHOD(extend);
  DEFINE_LIST_METHOD(index_of);
  DEFINE_LIST_METHOD(insert);
  DEFINE_LIST_METHOD(pop);
  DEFINE_LIST_METHOD(shift);
  DEFINE_LIST_METHOD(remove_at);
  DEFINE_LIST_METHOD(remove);
  DEFINE_LIST_METHOD(reverse);
  DEFINE_LIST_METHOD(sort);
  DEFINE_LIST_METHOD(contains);
  DEFINE_LIST_METHOD(delete);
  DEFINE_LIST_METHOD(first);
  DEFINE_LIST_METHOD(last);
  DEFINE_LIST_METHOD(is_empty);
  DEFINE_LIST_METHOD(take);
  DEFINE_LIST_METHOD(get);
  DEFINE_LIST_METHOD(compact);
  DEFINE_LIST_METHOD(unique);
  DEFINE_LIST_METHOD(zip);
  DEFINE_LIST_METHOD(to_dict);
  DEFINE_LIST_METHOD(__iter__);
  DEFINE_LIST_METHOD(__itern__);

  // dictionary methods
  DEFINE_DICT_METHOD(length);
  DEFINE_DICT_METHOD(add);
  DEFINE_DICT_METHOD(set);
  DEFINE_DICT_METHOD(clear);
  DEFINE_DICT_METHOD(clone);
  DEFINE_DICT_METHOD(compact);
  DEFINE_DICT_METHOD(contains);
  DEFINE_DICT_METHOD(extend);
  DEFINE_DICT_METHOD(get);
  DEFINE_DICT_METHOD(keys);
  DEFINE_DICT_METHOD(values);
  DEFINE_DICT_METHOD(remove);
  DEFINE_DICT_METHOD(assign);
  DEFINE_DICT_METHOD(is_empty);
  DEFINE_DICT_METHOD(find_key);
  DEFINE_DICT_METHOD(to_list);
  DEFINE_DICT_METHOD(has_attr);
  DEFINE_DICT_METHOD(__iter__);
  DEFINE_DICT_METHOD(__itern__);

  // file methods
  DEFINE_FILE_METHOD(exists);
  DEFINE_FILE_METHOD(close);
  DEFINE_FILE_METHOD(open);
  DEFINE_FILE_METHOD(read);
  DEFINE_FILE_METHOD(write);
  DEFINE_FILE_METHOD(number);
  DEFINE_FILE_METHOD(is_tty);
  DEFINE_FILE_METHOD(is_open);
  DEFINE_FILE_METHOD(is_closed);
  DEFINE_FILE_METHOD(flush);
  DEFINE_FILE_METHOD(stats);
  DEFINE_FILE_METHOD(symlink);
  DEFINE_FILE_METHOD(delete);
  DEFINE_FILE_METHOD(rename);
  DEFINE_FILE_METHOD(path);
  DEFINE_FILE_METHOD(abs_path);
  DEFINE_FILE_METHOD(copy);
  DEFINE_FILE_METHOD(truncate);
  DEFINE_FILE_METHOD(chmod);
  DEFINE_FILE_METHOD(set_times);
  DEFINE_FILE_METHOD(seek);
  DEFINE_FILE_METHOD(tell);
  DEFINE_FILE_METHOD(mode);
  DEFINE_FILE_METHOD(name);

  // bytes
  DEFINE_BYTES_METHOD(length);
  DEFINE_BYTES_METHOD(append);
  DEFINE_BYTES_METHOD(clone);
  DEFINE_BYTES_METHOD(extend);
  DEFINE_BYTES_METHOD(pop);
  DEFINE_BYTES_METHOD(remove);
  DEFINE_BYTES_METHOD(reverse);
  DEFINE_BYTES_METHOD(first);
  DEFINE_BYTES_METHOD(last);
  DEFINE_BYTES_METHOD(get);
  DEFINE_BYTES_METHOD(is_alpha);
  DEFINE_BYTES_METHOD(is_alnum);
  DEFINE_BYTES_METHOD(is_number);
  DEFINE_BYTES_METHOD(is_lower);
  DEFINE_BYTES_METHOD(is_upper);
  DEFINE_BYTES_METHOD(is_space);
  DEFINE_BYTES_METHOD(to_list);
  DEFINE_BYTES_METHOD(to_string);
  DEFINE_BYTES_METHOD(__iter__);
  DEFINE_BYTES_METHOD(__itern__);

#undef DEFINE_STRING_METHOD
#undef DEFINE_LIST_METHOD
#undef DEFINE_DICT_METHOD
#undef DEFINE_FILE_METHOD
#undef DEFINE_BYTES_METHOD
}

void init_vm(b_vm *vm) {

  reset_stack(vm);
  vm->compiler = NULL;
  vm->objects = NULL;
  vm->exception_class = NULL;
  vm->bytes_allocated = 0;
  vm->next_gc = 1024 * 1024; // 1mb // @TODO: Increase before going production.
  vm->is_repl = false;
  vm->is_calling_native = false;

  vm->gray_count = 0;
  vm->gray_capacity = 0;
  vm->gray_stack = NULL;
  vm->active_objects_count = 0;

  init_table(&vm->strings);
  init_table(&vm->globals);

  // object methods tables
  init_table(&vm->methods_string);
  init_table(&vm->methods_list);
  init_table(&vm->methods_dict);
  init_table(&vm->methods_file);
  init_table(&vm->methods_bytes);

  init_builtin_functions(vm);
  init_builtin_methods(vm);
  initialize_exceptions(vm);

  // this should be called once for the lifetime of an application
  curl_global_init(CURL_GLOBAL_ALL);
}

void free_vm(b_vm *vm) {
  // this should be called once for the lifetime of an application
  curl_global_cleanup();

  free_objects(vm);
  free_table(vm, &vm->strings);
  free_table(vm, &vm->globals);

  free_table(vm, &vm->methods_string);
  free_table(vm, &vm->methods_list);
  free_table(vm, &vm->methods_dict);
  free_table(vm, &vm->methods_file);
  free_table(vm, &vm->methods_bytes);
}

static bool call(b_vm *vm, b_obj *callee, b_obj_func *function, int arg_count) {
  // handle variadic arguments...
  if (function->is_variadic && arg_count >= function->arity - 1) {
    int va_args_start = arg_count - function->arity;
    b_obj_list *args_list = new_list(vm);

    for (int i = va_args_start; i >= 0; i--) {
      write_value_arr(vm, &args_list->items, peek(vm, i));
    }
    arg_count -= va_args_start;
    pop_n(vm, va_args_start + 1);
    push(vm, OBJ_VAL(args_list));
  } else if (!function->is_variadic && arg_count < function->arity) {
    for (; arg_count < function->arity; arg_count++) {
      push(vm, NIL_VAL);
    }
  }

  if (arg_count != function->arity) {
    if (function->is_variadic) {
      _runtime_error(vm, "expected at least %d arguments but got %d",
                     function->arity - 1, arg_count);
    } else {
      _runtime_error(vm, "expected %d arguments but got %d", function->arity,
                     arg_count);
    }
    return false;
  }

  if (vm->frame_count == FRAMES_MAX) {
    _runtime_error(vm, "stack overflow");
    return false;
  }

  b_call_frame *frame = &vm->frames[vm->frame_count++];
  frame->function = (b_obj *)callee;
  frame->ip = function->blob.code;

  frame->slots = vm->stack_top - arg_count - 1;
  return true;
}

static bool call_closure(b_vm *vm, b_obj_closure *closure, int arg_count) {
  return call(vm, (b_obj *)closure, closure->function, arg_count);
}

static bool call_function(b_vm *vm, b_obj_func *function, int arg_count) {
  return call(vm, (b_obj *)function, function, arg_count);
}

static bool call_value(b_vm *vm, b_value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_BOUND_METHOD: {
      b_obj_bound *bound = AS_BOUND(callee);
      vm->stack_top[-arg_count - 1] = bound->receiver;
      if (bound->method->type == OBJ_CLOSURE) {
        return call_closure(vm, (b_obj_closure *)bound->method, arg_count);
      } else {
        return call_function(vm, (b_obj_func *)bound->method, arg_count);
      }
    }

    case OBJ_CLASS: {
      b_obj_class *klass = AS_CLASS(callee);
      vm->stack_top[-arg_count - 1] = OBJ_VAL(new_instance(vm, klass));
      if (!IS_EMPTY(klass->initializer)) {
        if (IS_CLOSURE(klass->initializer)) {
          return call_closure(vm, AS_CLOSURE(klass->initializer), arg_count);
        } else if (IS_NATIVE(klass->initializer)) {
          return call_value(vm, klass->initializer, arg_count);
        } else {
          return call_function(vm, AS_FUNCTION(klass->initializer), arg_count);
        }
      } else if (arg_count != 0) {
        _runtime_error(vm, "%s constructor expects 0 arguments, %d given",
                       klass->name, arg_count);
        return false;
      }
      return true;
    }

    case OBJ_CLOSURE: {
      return call_closure(vm, AS_CLOSURE(callee), arg_count);
    }

    case OBJ_FUNCTION: {
      return call_function(vm, AS_FUNCTION(callee), arg_count);
    }

    case OBJ_NATIVE: {
      vm->is_calling_native = true;
      b_native_fn native = AS_NATIVE(callee)->function;
      b_value result = native(vm, arg_count, vm->stack_top - arg_count);

      if (IS_EMPTY(result)) {
        return false;
      }

      // clear active objects...
      if(vm->active_objects_count > 0) {
        pop_n(vm, vm->active_objects_count);
        vm->active_objects_count = 0;
      }

      vm->stack_top -= arg_count + 1;
      push(vm, result);

      vm->is_calling_native = false;
      return true;
    }

    default: // non callable
      break;
    }
  }
  _runtime_error(vm, "only functions and classes can be called");
  return false;
}

bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name,
                       int arg_count) {
  b_value method;
  if (!table_get(&klass->methods, OBJ_VAL(name), &method)) {
    if (!table_get(&klass->static_methods, OBJ_VAL(name), &method)) {
      _runtime_error(vm, "undefined method '%s' in %s", name->chars,
                     klass->name->chars);
    } else {
      _runtime_error(vm, "cannot call static method '%s' from instance of %s",
                     name->chars, klass->name->chars);
    }
    return false;
  }

  if (IS_NATIVE(method)) {
    return call_value(vm, method, arg_count);
  } else if (IS_CLOSURE(method)) {
    return call_closure(vm, AS_CLOSURE(method), arg_count);
  } else {
    return call_function(vm, AS_FUNCTION(method), arg_count);
  }
}

static bool invoke(b_vm *vm, b_obj_string *name, int arg_count) {
  b_value receiver = peek(vm, arg_count);

  b_value value;
  if (IS_INSTANCE(receiver)) {
    b_obj_instance *instance = AS_INSTANCE(receiver);

    if (table_get(&instance->fields, OBJ_VAL(name), &value)) {
      vm->stack_top[-arg_count - 1] = value;
      return call_value(vm, value, arg_count);
    }

    return invoke_from_class(vm, instance->klass, name, arg_count);
  } else if (IS_STRING(receiver)) {
    if (table_get(&vm->methods_string, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  } else if (IS_LIST(receiver)) {
    if (table_get(&vm->methods_list, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  } else if (IS_DICT(receiver)) {
    if (table_get(&vm->methods_dict, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  } else if (IS_FILE(receiver)) {
    if (table_get(&vm->methods_file, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  } else if (IS_BYTES(receiver)) {
    if (table_get(&vm->methods_bytes, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  } else if (IS_CLASS(receiver)) {
    if (table_get(&AS_CLASS(receiver)->static_methods, OBJ_VAL(name), &value) ||
        table_get(&AS_CLASS(receiver)->static_fields, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }
  }

  _runtime_error(vm, "cannot call method %s on object of type %s", name->chars,
                 value_type(receiver));
  return false;
}

static bool bind_method(b_vm *vm, b_obj_class *klass, b_obj_string *name) {
  b_value method;
  if (!table_get(&klass->methods, OBJ_VAL(name), &method)) {
    _runtime_error(vm, "undefined property '%s'", name->chars);
    return false;
  }

  b_obj_bound *bound = new_bound_method(vm, peek(vm, 0), AS_OBJ(method));
  pop(vm);
  push(vm, OBJ_VAL(bound));
  return true;
}

static b_obj_up_value *capture_up_value(b_vm *vm, b_value *local) {
  b_obj_up_value *prev_up_value = NULL;
  b_obj_up_value *up_value = vm->open_up_values;

  while (up_value != NULL && up_value->location > local) {
    prev_up_value = up_value;
    up_value = up_value->next;
  }

  if (up_value != NULL && up_value->location == local)
    return up_value;

  b_obj_up_value *created_up_value = new_up_value(vm, local);
  created_up_value->next = up_value;

  if (prev_up_value == NULL) {
    vm->open_up_values = created_up_value;
  } else {
    prev_up_value->next = created_up_value;
  }

  return created_up_value;
}

static void close_up_values(b_vm *vm, const b_value *last) {
  while (vm->open_up_values != NULL && vm->open_up_values->location >= last) {
    b_obj_up_value *up_value = vm->open_up_values;
    up_value->closed = *up_value->location;
    up_value->location = &up_value->closed;
    vm->open_up_values = up_value->next;
  }
}

static void define_method(b_vm *vm, b_obj_string *name, bool is_static) {
  b_value method = peek(vm, 0);
  b_obj_class *klass = AS_CLASS(peek(vm, 1));
  if (!is_static) {
    table_set(vm, &klass->methods, OBJ_VAL(name), method);
  } else {
    table_set(vm, &klass->static_methods, OBJ_VAL(name), method);
  }
  if (name == klass->name && !is_static) {
    klass->initializer = method;
  }
  pop(vm);
}

static void define_property(b_vm *vm, b_obj_string *name, bool is_static) {
  b_value property = peek(vm, 0);
  b_obj_class *klass = AS_CLASS(peek(vm, 1));
  if (!is_static) {
    table_set(vm, &klass->fields, OBJ_VAL(name), property);
  } else {
    table_set(vm, &klass->static_fields, OBJ_VAL(name), property);
  }
  pop(vm);
}

bool is_falsey(b_value value) {
  if (IS_BOOL(value))
    return IS_BOOL(value) && !AS_BOOL(value);
  if (IS_NIL(value) || IS_EMPTY(value))
    return true;

  // -1 is the number equivalent of false in Birdy
  if (IS_NUMBER(value))
    return AS_NUMBER(value) < 0;

  // Non-empty strings are true, empty strings are false.
  if (IS_STRING(value))
    return strlen(AS_STRING(value)->chars) < 1;

  // Non-empty lists are true, empty lists are false.
  if (IS_LIST(value))
    return AS_LIST(value)->items.count == 0;

  // Non-empty dicts are true, empty dicts are false.
  if (IS_DICT(value))
    return AS_DICT(value)->names.count == 0;

  // All classes are true
  // All closures are true
  // All bound methods are true
  // All functions are in themselves true if you do not account for what they
  // return.
  return false;
}

bool is_instance_of(b_obj_class *klass1, char *klass2_name) {
  while (klass1 != NULL) {
    if (memcmp(klass1->name->chars, klass2_name, klass1->name->length) == 0) {
      return true;
    }
    klass1 = klass1->superclass;
  }

  return false;
}

static void print_exception(b_vm *vm, b_obj_instance *exception) {
  b_value message, trace;
  if (table_get(&exception->fields, STRING_L_VAL("message", 7), &message) &&
      table_get(&exception->klass->methods, STRING_L_VAL("trace", 5), &trace)) {
    fprintf(stderr, "Unhandled Exception: %s: %s\n",
            exception->klass->name->chars, value_to_string(vm, message));
    if (call_value(vm, trace, 0)) {
      // while value to string may be heavy here, we can't make
      // any assumption that the user will not try to override
      // the trace method and return a value we do not anticipate.
      fprintf(stderr, "%s\n", value_to_string(vm, pop(vm)));
    }
    vm->frame_count = 0;
  } else {
    _runtime_error(vm, "invalid Exception or Exception subclass instance");
  }
}

void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value) {
  write_value_arr(vm, &dict->names, key);
  table_set(vm, &dict->items, key, value);
}

bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value) {
  /* // this will be easier to search than the entire tables
  // if the key doesn't exist.
  if (dict->names.count < (int)sizeof(uint8_t)) {
    int i;
    bool found = false;
    for (i = 0; i < dict->names.count; i++) {
      if (values_equal(dict->names.values[i], key)) {
        found = true;
        break;
      }
    }

    if (!found)
      return false;
  } */
  return table_get(&dict->items, key, value);
}

bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value) {
#if defined USE_NAN_BOXING && USE_NAN_BOXING
  bool found = false;
  for (int i = 0; i < dict->names.count; i++) {
    if (values_equal(dict->names.values[i], key))
      found = true;
  }
  if (!found)
    write_value_arr(vm, &dict->names, key); // add key if it doesn't exist.
#else
  b_value temp_value;
  if (!table_get(&dict->items, key, &temp_value)) {
    write_value_arr(vm, &dict->names, key); // add key if it doesn't exist.
  }
#endif
  return table_set(vm, &dict->items, key, value);
}

static b_obj_string *multiply_string(b_vm *vm, b_obj_string *str,
                                     double number) {
  int times = (int)number;

  if (times <= 0) // 'str' * 0 == '', 'str' * -1 == ''
    return copy_string(vm, "", 0);
  else if (times == 1) // 'str' * 1 == 'str'
    return str;

  int total_length = str->length * times;
  char *result = calloc(total_length + 1, sizeof(char));

  for (int i = 0; i < times; i++) {
    memcpy(result + (str->length * i), str->chars, str->length);
  }
  result[total_length] = '\0';
  return take_string(vm, result, total_length);
}

static b_obj_list *add_list(b_vm *vm, b_obj_list *a, b_obj_list *b) {
  b_obj_list *list = new_list(vm);

  for (int i = 0; i < a->items.count; i++) {
    write_value_arr(vm, &list->items, a->items.values[i]);
  }

  for (int i = 0; i < b->items.count; i++) {
    write_value_arr(vm, &list->items, b->items.values[i]);
  }

  return list;
}

static b_obj_bytes *add_bytes(b_vm *vm, b_obj_bytes *a, b_obj_bytes *b) {
  b_obj_bytes *bytes = new_bytes(vm, a->bytes.count + b->bytes.count);

  memcpy(bytes->bytes.bytes, a->bytes.bytes,
         a->bytes.count * sizeof(unsigned char *));
  memcpy(bytes->bytes.bytes + a->bytes.count, b->bytes.bytes,
         b->bytes.count * sizeof(unsigned char *));

  return bytes;
}

static b_obj_list *multiply_list(b_vm *vm, b_obj_list *a, b_obj_list *new_list,
                                 int times) {
  for (int i = 0; i < times; i++) {
    for (int j = 0; j < a->items.count; j++) {
      write_value_arr(vm, &new_list->items, a->items.values[j]);
    }
  }

  return new_list;
}

static bool dict_get_index(b_vm *vm, b_obj_dict *dict, bool will_assign) {
  b_value index;
  if (!will_assign) {
    pop(vm); // discard upper... we won't need it so gc can free it.
    index = peek(vm, 0);
  } else {
    index = peek(vm, 1);
  }

  b_value result;
  if (dict_get_entry(dict, index, &result)) {
    if (!will_assign) {
      pop_n(vm, 2); // we can safely get rid of the index from the stack
    }
    push(vm, result);
    return true;
  }

  _runtime_error(vm, "invalid index %s", value_to_string(vm, index));
  return false;
}

static bool string_get_index(b_vm *vm, b_obj_string *string, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (IS_NIL(upper)) {
    if (!IS_NUMBER(lower)) {
      _runtime_error(vm, "strings are numerically indexed");
      return false;
    }

    if (!will_assign) {
      pop(vm); // discard upper... we won't need it so gc can free it.
    }
    int index = AS_NUMBER(lower);
    int real_index = index;
    if (index < 0)
      index = string->utf8_length + index;

    if (index < string->utf8_length && index >= 0) {
      if (!will_assign) {
        // we can safely get rid of the index from the stack
        pop_n(vm, 2); // +1 for the list itself
      }

      int start = index, end = index + 1;
      utf8slice(string->chars, &start, &end);

      push(vm, STRING_L_VAL(string->chars + start, (int)(end - start)));
      return true;
    } else {
      _runtime_error(vm, "string index %d out of range", real_index);
      return false;
    }
  } else {
    if (!IS_NUMBER(lower) || !IS_NUMBER(upper)) {
      _runtime_error(vm, "string are numerically indexed");
      return false;
    }

    int lower_index = AS_NUMBER(lower);
    int upper_index = AS_NUMBER(upper);

    if (lower_index < 0 ||
        (upper_index < 0 && ((string->utf8_length + upper_index) < 0))) {
      // always return an empty list...
      if (!will_assign) {
        pop_n(vm, 3); // +1 for the list itself
      }
      push(vm, STRING_L_VAL("", 0));
      return true;
    }

    if (upper_index < 0)
      upper_index = string->utf8_length + upper_index;

    if (upper_index > string->utf8_length)
      upper_index = string->utf8_length;

    if (!will_assign) {
      pop_n(vm, 3); // +1 for the list itself
    }

    int start = lower_index, end = upper_index;
    utf8slice(string->chars, &start, &end);

    push(vm, STRING_L_VAL(string->chars + start, (int)(end - start)));
    return true;
  }
}

static bool bytes_get_index(b_vm *vm, b_obj_bytes *bytes, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (IS_NIL(upper)) {
    if (!IS_NUMBER(lower)) {
      _runtime_error(vm, "bytes are numerically indexed");
      return false;
    }

    if (!will_assign) {
      pop(vm); // discard upper... we won't need it so gc can free it.
    }
    int index = AS_NUMBER(lower);
    int real_index = index;
    if (index < 0)
      index = bytes->bytes.count + index;

    if (index < bytes->bytes.count && index >= 0) {
      if (!will_assign) {
        // we can safely get rid of the index from the stack
        pop_n(vm, 2); // +1 for the list itself
      }

      push(vm, NUMBER_VAL((int)bytes->bytes.bytes[index]));
      return true;
    } else {
      _runtime_error(vm, "bytes index %d out of range", real_index);
      return false;
    }
  } else {
    if (!IS_NUMBER(lower) || !IS_NUMBER(upper)) {
      _runtime_error(vm, "bytes are numerically indexed");
      return false;
    }

    int lower_index = AS_NUMBER(lower);
    int upper_index = AS_NUMBER(upper);

    if (lower_index < 0 ||
        (upper_index < 0 && ((bytes->bytes.count + upper_index) < 0))) {
      // always return an empty list...
      if (!will_assign) {
        pop_n(vm, 3); // +1 for the list itself
      }
      _runtime_error(vm, "bytes index %d out of range",
                     lower_index < 0 ? lower_index : upper_index);
      return false;
    }

    if (upper_index < 0)
      upper_index = bytes->bytes.count + upper_index;

    if (upper_index > bytes->bytes.count)
      upper_index = bytes->bytes.count;

    if (!will_assign) {
      pop_n(vm, 3); // +1 for the list itself
    }
    push(vm, OBJ_VAL(copy_bytes(vm, bytes->bytes.bytes + lower_index,
                                upper_index - lower_index)));
    return true;
  }
}

static bool list_get_index(b_vm *vm, b_obj_list *list, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (IS_NIL(upper)) {
    if (!IS_NUMBER(lower)) {
      _runtime_error(vm, "list are numerically indexed");
      return false;
    }

    if (!will_assign) {
      pop(vm); // discard upper... we won't need it so gc can free it.
    }
    int index = AS_NUMBER(lower);
    int real_index = index;
    if (index < 0)
      index = list->items.count + index;

    if (index < list->items.count && index >= 0) {
      if (!will_assign) {
        // we can safely get rid of the index from the stack
        pop_n(vm, 2); // +1 for the list itself
      }

      push(vm, list->items.values[index]);
      return true;
    } else {
      _runtime_error(vm, "list index %d out of range", real_index);
      return false;
    }
  } else {
    if (!IS_NUMBER(lower) || !IS_NUMBER(upper)) {
      _runtime_error(vm, "list are numerically indexed");
      return false;
    }

    int lower_index = AS_NUMBER(lower);
    int upper_index = AS_NUMBER(upper);

    if (lower_index < 0 ||
        (upper_index < 0 && ((list->items.count + upper_index) < 0))) {
      // always return an empty list...
      if (!will_assign) {
        pop_n(vm, 3); // +1 for the list itself
      }
      push(vm, OBJ_VAL(new_list(vm)));
      return true;
    }

    if (upper_index < 0)
      upper_index = list->items.count + upper_index;

    if (upper_index > list->items.count)
      upper_index = list->items.count;

    b_obj_list *n_list = new_list(vm);

    for (int i = lower_index; i < upper_index; i++) {
      write_value_arr(vm, &n_list->items, list->items.values[i]);
    }

    if (!will_assign) {
      pop_n(vm, 3); // +1 for the list itself
    }
    push(vm, OBJ_VAL(n_list));
    return true;
  }
}

static void dict_set_index(b_vm *vm, b_obj_dict *dict, b_value index,
                           b_value value) {
  dict_set_entry(vm, dict, index, value);
  pop_n(vm, 4); // pop the value, nil, index and dict out

  // leave the value on the stack for consumption
  // e.g. variable = dict[index] = 10
  push(vm, value);
}

static bool list_set_index(b_vm *vm, b_obj_list *list, b_value index,
                           b_value value) {
  if (!IS_NUMBER(index)) {
    _runtime_error(vm, "list are numerically indexed");
    return false;
  }

  int _position = AS_NUMBER(index);
  int position = _position < 0 ? list->items.count + _position : _position;

  if (position < list->items.count && position > -(list->items.count)) {
    list->items.values[position] = value;
    pop_n(vm, 4); // pop the value, nil, index and list out

    // leave the value on the stack for consumption
    // e.g. variable = list[index] = 10
    push(vm, value);
    return true;
  }

  _runtime_error(vm, "lists index %d out of range", _position);
  return false;
}

static bool bytes_set_index(b_vm *vm, b_obj_bytes *bytes, b_value index,
                            b_value value) {
  if (!IS_NUMBER(index)) {
    _runtime_error(vm, "bytes are numerically indexed");
    return false;
  } else if (!IS_NUMBER(value) || AS_NUMBER(value) < 0 ||
             AS_NUMBER(value) > 255) {
    _runtime_error(vm, "invalid byte. bytes are numbers between 0 and 255.");
    return false;
  }

  int _position = AS_NUMBER(index);
  int byte = AS_NUMBER(value);

  int position = _position < 0 ? bytes->bytes.count + _position : _position;

  if (position < bytes->bytes.count && position > -(bytes->bytes.count)) {
    bytes->bytes.bytes[position] = (unsigned char)byte;
    pop_n(vm, 4); // pop the value, nil, index and bytes out

    // leave the value on the stack for consumption
    // e.g. variable = bytes[index] = 10
    push(vm, value);
    return true;
  }

  _runtime_error(vm, "bytes index %d out of range", _position);
  return false;
}

static bool concatenate(b_vm *vm) {
  b_value _b = peek(vm, 0);
  b_value _a = peek(vm, 1);

  if (IS_NIL(_a)) {
    pop_n(vm, 2);
    push(vm, _b);
  } else if (IS_NIL(_b)) {
    pop(vm);
  } else if (IS_NUMBER(_a)) {
    double a = AS_NUMBER(_a);

    char num_str[27]; // + 1 for null terminator
    int num_length = sprintf(num_str, NUMBER_FORMAT, a);

    b_obj_string *b = AS_STRING(_b);

    int length = num_length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, num_str, num_length);
    memcpy(chars + num_length, b->chars, b->length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = utf8len(result->chars);

    pop_n(vm, 2);
    push(vm, OBJ_VAL(result));
  } else if (IS_NUMBER(_b)) {
    b_obj_string *a = AS_STRING(_a);
    double b = AS_NUMBER(_b);

    char num_str[27]; // + 1 for null terminator
    int num_length = sprintf(num_str, NUMBER_FORMAT, b);

    int length = num_length + a->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, num_str, num_length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = utf8len(result->chars);

    pop_n(vm, 2);
    push(vm, OBJ_VAL(result));
  } else if (IS_STRING(_a) && IS_STRING(_b)) {
    b_obj_string *b = AS_STRING(_b);
    b_obj_string *a = AS_STRING(_a);

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = utf8len(result->chars);

    pop_n(vm, 2);
    push(vm, OBJ_VAL(result));
  } else {
    return false;
  }

  return true;
}

static int floor_div(double a, double b) {
  int d = (int)a / (int)b;
  return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

b_ptr_result run(b_vm *vm) {

  b_call_frame *frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT()                                                        \
  (get_frame_function(frame)->blob.constants.values[READ_SHORT()])

#define READ_STRING() (AS_STRING(READ_CONSTANT()))

#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      _runtime_error(vm, "unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
      EXIT_VM();                                                               \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(a op b));                                                    \
  } while (false)

#define BINARY_BIT_OP(type, op)                                                \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      _runtime_error(vm, "unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
      EXIT_VM();                                                               \
    }                                                                          \
    int b = AS_NUMBER(pop(vm));                                                \
    int a = AS_NUMBER(pop(vm));                                                \
    push(vm, type((double)(a op b)));                                          \
  } while (false)

#define BINARY_MOD_OP(type, op)                                                \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      _runtime_error(vm, "unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
      EXIT_VM();                                                               \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(op(a, b)));                                                  \
  } while (false)

  for (;;) {

#if defined DEBUG_TRACE_EXECUTION && DEBUG_TRACE_EXECUTION
    printf("          ");
    for (b_value *slot = vm->stack; slot < vm->stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(
        &get_frame_function(frame)->blob,
        (int)(frame->ip - get_frame_function(frame)->blob.code));
#endif

    uint8_t instruction;

    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      b_value constant = READ_CONSTANT();
      push(vm, constant);
      break;
    }

    case OP_ADD: {
      if (IS_STRING(peek(vm, 0)) || IS_STRING(peek(vm, 1))) {
        if (!concatenate(vm)) {
          runtime_error("unsupported operand + for %s and %s",
                        value_type(peek(vm, 0)), value_type(peek(vm, 1)));
        }
      } else if (IS_LIST(peek(vm, 0)) && IS_LIST(peek(vm, 1))) {
        b_value result =
            OBJ_VAL(add_list(vm, AS_LIST(peek(vm, 1)), AS_LIST(peek(vm, 0))));
        pop_n(vm, 2);
        push(vm, result);
      } else if (IS_BYTES(peek(vm, 0)) && IS_BYTES(peek(vm, 1))) {
        b_value result = OBJ_VAL(
            add_bytes(vm, AS_BYTES(peek(vm, 1)), AS_BYTES(peek(vm, 0))));
        pop_n(vm, 2);
        push(vm, result);
      } else {
        BINARY_OP(NUMBER_VAL, +);
      }
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY: {
      if (IS_STRING(peek(vm, 1)) && IS_NUMBER(peek(vm, 0))) {
        double number = AS_NUMBER(peek(vm, 0));
        b_obj_string *string = AS_STRING(peek(vm, 1));
        b_value result = OBJ_VAL(multiply_string(vm, string, number));
        pop_n(vm, 2);
        push(vm, result);
        break;
      } else if (IS_LIST(peek(vm, 1)) && IS_NUMBER(peek(vm, 0))) {
        int number = (int)AS_NUMBER(pop(vm));
        b_obj_list *list = AS_LIST(peek(vm, 0));
        b_obj_list *n_list = new_list(vm);
        push(vm, OBJ_VAL(n_list));
        b_value result = OBJ_VAL(multiply_list(vm, list, n_list, number));
        pop_n(vm, 2);
        push(vm, result);
        break;
      }
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_REMINDER: {
      BINARY_MOD_OP(NUMBER_VAL, fmod);
      break;
    }
    case OP_POW: {
      BINARY_MOD_OP(NUMBER_VAL, pow);
      break;
    }
    case OP_F_DIVIDE: {
      BINARY_MOD_OP(NUMBER_VAL, floor_div);
      break;
    }
    case OP_NEGATE: {
      if (!IS_NUMBER(peek(vm, 0))) {
        runtime_error("operator - not defined for object of type %s",
                      value_type(peek(vm, 0)));
      }
      push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
      break;
    }
    case OP_BIT_NOT: {
      if (!IS_NUMBER(peek(vm, 0))) {
        runtime_error("operator ~ not defined for object of type %s",
                      value_type(peek(vm, 0)));
      }
      push(vm, INTEGER_VAL(~((int)AS_NUMBER(pop(vm)))));
      break;
    }
    case OP_AND: {
      BINARY_BIT_OP(NUMBER_VAL, &);
      break;
    }
    case OP_OR: {
      BINARY_BIT_OP(NUMBER_VAL, |);
      break;
    }
    case OP_XOR: {
      BINARY_BIT_OP(NUMBER_VAL, ^);
      break;
    }
    case OP_LSHIFT: {
      BINARY_BIT_OP(NUMBER_VAL, <<);
      break;
    }
    case OP_RSHIFT: {
      BINARY_BIT_OP(NUMBER_VAL, >>);
      break;
    }
    case OP_ONE: {
      push(vm, NUMBER_VAL(1));
      break;
    }

      // comparisons
    case OP_EQUAL: {
      b_value b = pop(vm);
      b_value a = pop(vm);
      push(vm, BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }

    case OP_NOT:
      push(vm, BOOL_VAL(is_falsey(pop(vm))));
      break;
    case OP_NIL:
      push(vm, NIL_VAL);
      break;
    case OP_TRUE:
      push(vm, BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(vm, BOOL_VAL(false));
      break;

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(vm, 0))) {
        frame->ip += offset;
      }
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_ECHO: {
      if (vm->is_repl) {
        echo_value(peek(vm, 0));
      } else {
        print_value(peek(vm, 0));
      }
      pop(vm);
      printf("\n"); // @TODO: remove when library function print is ready
      break;
    }

    case OP_STRINGIFY: {
      if (!IS_STRING(peek(vm, 0))) {
        char *value = value_to_string(vm, pop(vm));
        push(vm, OBJ_VAL(take_string(vm, value, (int)strlen(value))));
      }
      break;
    }

    case OP_DUP: {
      push(vm, peek(vm, 0));
      break;
    }
    case OP_POP: {
      pop(vm);
      break;
    }
    case OP_POP_N: {
      pop_n(vm, READ_SHORT());
      break;
    }
    case OP_CLOSE_UP_VALUE: {
      close_up_values(vm, vm->stack_top - 1);
      pop(vm);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      b_obj_string *name = READ_STRING();
      table_set(vm, &vm->globals, OBJ_VAL(name), peek(vm, 0));
      pop(vm);

#if defined DEBUG_TABLE && DEBUG_TABLE
      table_print(&vm->globals);
#endif
      break;
    }

    case OP_GET_GLOBAL: {
      b_obj_string *name = READ_STRING();
      b_value value;
      if (!table_get(&vm->globals, OBJ_VAL(name), &value)) {
        runtime_error("%s is undefined in this scope", name->chars);
      }
      push(vm, value);
      break;
    }

    case OP_SET_GLOBAL: {
      b_obj_string *name = READ_STRING();
      if (table_set(vm, &vm->globals, OBJ_VAL(name), peek(vm, 0))) {
        table_delete(&vm->globals, OBJ_VAL(name));
        runtime_error("%s is undefined in this scope", name->chars);
      }
      break;
    }

    case OP_GET_LOCAL: {
      uint16_t slot = READ_SHORT();
      push(vm, frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint16_t slot = READ_SHORT();
      frame->slots[slot] = peek(vm, 0);
      break;
    }

    case OP_GET_PROPERTY: {
      if (!IS_INSTANCE(peek(vm, 0)) && !IS_DICT(peek(vm, 0)) &&
          !IS_LIST(peek(vm, 0)) && !IS_BYTES(peek(vm, 0)) &&
          !IS_FILE(peek(vm, 0)) && !IS_STRING(peek(vm, 0)) &&
          !IS_CLASS(peek(vm, 0))) {
        runtime_error("object of type %s does not carry properties",
                      value_type(peek(vm, 0)));
      }

      b_obj_string *name = READ_STRING();

      if (IS_INSTANCE(peek(vm, 0))) {

        b_obj_instance *instance = AS_INSTANCE(peek(vm, 0));
        b_value value;
        if (table_get(&instance->fields, OBJ_VAL(name), &value)) {
          pop(vm); // pop the instance...
          push(vm, value);
          break;
        }

        if (!bind_method(vm, instance->klass, name)) {
          EXIT_VM();
        } else {
          break;
        }
      } else if (IS_DICT(peek(vm, 0))) {
        b_value value;
        if (table_get(&AS_DICT(peek(vm, 0))->items, OBJ_VAL(name), &value) ||
            table_get(&vm->methods_dict, OBJ_VAL(name), &value)) {
          pop(vm); // pop the dictionary...
          push(vm, value);
          break;
        }
      } else if (IS_LIST(peek(vm, 0))) {
        b_value value;
        if (table_get(&vm->methods_list, OBJ_VAL(name), &value)) {
          pop(vm); // pop the list...
          push(vm, value);
          break;
        }
      } else if (IS_BYTES(peek(vm, 0))) {
        b_value value;
        if (table_get(&vm->methods_bytes, OBJ_VAL(name), &value)) {
          pop(vm); // pop the bytes...
          push(vm, value);
          break;
        }
      } else if (IS_FILE(peek(vm, 0))) {
        b_value value;
        if (table_get(&vm->methods_file, OBJ_VAL(name), &value)) {
          pop(vm); // pop the file...
          push(vm, value);
          break;
        }
      } else if (IS_STRING(peek(vm, 0))) {
        b_value value;
        if (table_get(&vm->methods_string, OBJ_VAL(name), &value)) {
          pop(vm); // pop the string...
          push(vm, value);
          break;
        }
      } else if (IS_CLASS(peek(vm, 0))) {
        b_value value;
        if (table_get(&AS_CLASS(peek(vm, 0))->static_methods, OBJ_VAL(name),
                      &value) ||
            table_get(&AS_CLASS(peek(vm, 0))->static_fields, OBJ_VAL(name),
                      &value)) {
          pop(vm); // pop the class...
          push(vm, value);
          break;
        }
      }

      if (IS_CLASS(peek(vm, 0))) {
        runtime_error(
            "class %s does not have a static field or method named %s",
            AS_CLASS(peek(vm, 0))->name->chars, name->chars);
      } else if (IS_INSTANCE(peek(vm, 0))) {
        runtime_error(
            "instance of class %s %s does not have a field or method named %s",
            AS_INSTANCE(peek(vm, 0))->klass->name->chars, name->chars);
      } else {
        runtime_error("object of type %s does not have a property %s",
                      value_type(peek(vm, 0)), name->chars);
      }
    }
    case OP_SET_PROPERTY: {
      if (!IS_INSTANCE(peek(vm, 1))) {
        runtime_error("object of type %s can not carry properties",
                      value_type(peek(vm, 1)));
      }

      b_obj_instance *instance = AS_INSTANCE(peek(vm, 1));
      table_set(vm, &instance->fields, OBJ_VAL(READ_STRING()), peek(vm, 0));

      b_value value = pop(vm);
      pop(vm); // removing the instance object
      push(vm, value);
      break;
    }

    case OP_CLOSURE: {
      b_obj_func *function = AS_FUNCTION(READ_CONSTANT());
      b_obj_closure *closure = new_closure(vm, function);
      push(vm, OBJ_VAL(closure));

      for (int i = 0; i < closure->up_value_count; i++) {
        uint8_t is_local = READ_BYTE();
        int index = READ_SHORT();

        if (is_local) {
          closure->up_values[i] = capture_up_value(vm, frame->slots + index);
        } else {
          closure->up_values[i] =
              ((b_obj_closure *)frame->function)->up_values[index];
        }
      }

      break;
    }
    case OP_GET_UP_VALUE: {
      int index = READ_SHORT();
      push(vm, *((b_obj_closure *)frame->function)->up_values[index]->location);
      break;
    }
    case OP_SET_UP_VALUE: {
      int index = READ_SHORT();
      *((b_obj_closure *)frame->function)->up_values[index]->location =
          peek(vm, 0);
      break;
    }

    case OP_CALL: {
      int arg_count = READ_BYTE();
      if (!call_value(vm, peek(vm, arg_count), arg_count)) {
        EXIT_VM();
      }
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }
    case OP_INVOKE: {
      b_obj_string *method = READ_STRING();
      int arg_count = READ_BYTE();
      if (!invoke(vm, method, arg_count)) {
        EXIT_VM();
      }
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    case OP_CLASS: {
      b_obj_string *name = READ_STRING();
      push(vm, OBJ_VAL(new_class(vm, name)));
      break;
    }
    case OP_METHOD: {
      b_obj_string *name = READ_STRING();
      bool is_static = READ_BYTE() == 1;
      define_method(vm, name, is_static);
      break;
    }
    case OP_CLASS_PROPERTY: {
      b_obj_string *name = READ_STRING();
      int is_static = READ_BYTE();
      define_property(vm, name, is_static == 1);
      break;
    }
    case OP_INHERIT: {
      if (!IS_CLASS(peek(vm, 1))) {
        runtime_error("cannot inherit from non-class object");
      }

      b_obj_class *superclass = AS_CLASS(peek(vm, 1));
      b_obj_class *subclass = AS_CLASS(peek(vm, 0));
      table_add_all(vm, &superclass->fields, &subclass->fields);
      table_add_all(vm, &superclass->methods, &subclass->methods);
      subclass->superclass = superclass;
      pop(vm); // pop the subclass
      break;
    }
    case OP_GET_SUPER: {
      b_obj_string *name = READ_STRING();
      b_obj_instance *instance = AS_INSTANCE(peek(vm, 0));
      if (!bind_method(vm, instance->klass->superclass, name)) {
        EXIT_VM();
      }
      break;
    }
    case OP_SUPER_INVOKE: {
      b_obj_string *method = READ_STRING();
      int arg_count = READ_BYTE();
      b_obj_instance *instance = AS_INSTANCE(peek(vm, 0));
      if (!invoke_from_class(vm, instance->klass->superclass, method,
                             arg_count)) {
        EXIT_VM();
      }
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    case OP_LIST: {
      int count = READ_SHORT();
      b_obj_list *list = new_list(vm);
      vm->stack_top[-count - 1] = OBJ_VAL(list);

      for (int i = count - 1; i >= 0; i--) {
        write_list(vm, list, peek(vm, i)); // +1 to skip the list
      }
      pop_n(vm, count);
      break;
    }
    case OP_RANGE: {
      b_value _upper = peek(vm, 0), _lower = peek(vm, 1);

      if (!IS_NUMBER(_upper) || !IS_NUMBER(_lower)) {
        runtime_error("invalid range boundaries");
      }

      double lower = AS_NUMBER(_lower), upper = AS_NUMBER(_upper);

      b_obj_list *list = new_list(vm);
      pop_n(vm, 2);
      push(vm, OBJ_VAL(list));

      if (upper > lower) {
        for (int i = (int)lower; i < upper; i++) {
          write_list(vm, list, NUMBER_VAL(i));
        }
      } else if (lower > upper) {
        for (int i = (int)lower; i > upper; i--) {
          write_list(vm, list, NUMBER_VAL(i));
        }
      }
      break;
    }
    case OP_DICT: {
      int count = READ_SHORT() * 2; // 1 for key, 1 for value
      b_obj_dict *dict = new_dict(vm);
      vm->stack_top[-count - 1] = OBJ_VAL(dict);

      for (int i = 0; i < count; i += 2) {
        b_value name = vm->stack_top[-count + i];
        b_value value = vm->stack_top[-count + i + 1];
        dict_add_entry(vm, dict, name, value);
      }
      pop_n(vm, count);
      break;
    }
    case OP_GET_INDEX: {
      int will_assign = READ_BYTE();

      if (!IS_STRING(peek(vm, 2)) && !IS_LIST(peek(vm, 2)) &&
          !IS_DICT(peek(vm, 2)) && !IS_BYTES(peek(vm, 2))) {
        runtime_error("type of %s is not a valid iterable",
                      value_type(peek(vm, 2)));
      }

      if (IS_STRING(peek(vm, 2))) {
        if (!string_get_index(vm, AS_STRING(peek(vm, 2)),
                              will_assign == 1 ? true : false)) {
          EXIT_VM();
        } else {
          break;
        }
      } else if (IS_LIST(peek(vm, 2))) {
        if (!list_get_index(vm, AS_LIST(peek(vm, 2)),
                            will_assign == 1 ? true : false)) {
          EXIT_VM();
        } else {
          break;
        }
      } else if (IS_BYTES(peek(vm, 2))) {
        if (!bytes_get_index(vm, AS_BYTES(peek(vm, 2)),
                             will_assign == 1 ? true : false)) {
          EXIT_VM();
        } else {
          break;
        }
      } else if (IS_DICT(peek(vm, 2)) && IS_NIL(peek(vm, 0))) {
        if (!dict_get_index(vm, AS_DICT(peek(vm, 2)),
                            will_assign == 1 ? true : false)) {
          EXIT_VM();
        } else {
          break;
        }
      }

      runtime_error("invalid index %s", value_to_string(vm, peek(vm, 0)));
    }
    case OP_SET_INDEX: {
      if (!IS_LIST(peek(vm, 3)) && !IS_DICT(peek(vm, 3)) &&
          !IS_BYTES(peek(vm, 3))) {
        if (!IS_STRING(peek(vm, 3))) {
          runtime_error("type of %s is not a valid iterable",
                        value_type(peek(vm, 3)));
        } else {
          runtime_error("strings do not support object assignment");
        }
      }

      b_value value = peek(vm, 0);
      b_value index = peek(vm, 2); // since peek 1 will be nil

      if (IS_LIST(peek(vm, 3))) {
        if (!list_set_index(vm, AS_LIST(peek(vm, 3)), index, value)) {
          EXIT_VM();
        }
      } else if (IS_BYTES(peek(vm, 3))) {
        if (!bytes_set_index(vm, AS_BYTES(peek(vm, 3)), index, value)) {
          EXIT_VM();
        }
      } else if (IS_DICT(peek(vm, 3))) {
        dict_set_index(vm, AS_DICT(peek(vm, 3)), index, value);
        break;
      }
      break;
    }

    case OP_RETURN: {
      b_value result = pop(vm);

      close_up_values(vm, frame->slots);

      vm->frame_count--;
      if (vm->frame_count == 0) {
        pop(vm);
        return PTR_OK;
      }

      vm->stack_top = frame->slots;
      push(vm, result);

      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    case OP_CALL_IMPORT: {
      b_obj_func *function = AS_FUNCTION(READ_CONSTANT());
      call_function(vm, function, 0);
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    case OP_FINISH_MODULE: {
      b_obj_func *function = AS_FUNCTION(READ_CONSTANT());
      // if it is a native module, attach c codes to cask methods
      bind_native_modules(vm, function->name, function->file);
      break;
    }

    case OP_ASSERT: {
      b_value message = pop(vm);
      b_value expression = pop(vm);
      if (is_falsey(expression)) {
        if (!IS_NIL(message)) {
          runtime_error("AssertionError: %s", value_to_string(vm, message));
        } else {
          runtime_error("AssertionError");
        }
      }
      break;
    }

    case OP_DIE: {
      if (!IS_INSTANCE(peek(vm, 0)) ||
          !is_instance_of(AS_INSTANCE(peek(vm, 0))->klass,
                          vm->exception_class->name->chars)) {
        runtime_error("instance of Exception expected");
      }

      if (vm->catch_frame == NULL) {
        print_exception(vm, AS_INSTANCE(peek(vm, 0)));
        return PTR_RUNTIME_ERR;
      } else {
        frame = vm->catch_frame->frame;
        frame->ip =
            get_frame_function(frame)->blob.code + vm->catch_frame->offset;
        break;
      }
    }

    case OP_TRY: {
      b_catch_frame *catch_frame =
          (b_catch_frame *)malloc(sizeof(b_catch_frame));
      catch_frame->frame = frame;
      catch_frame->offset = READ_SHORT();
      catch_frame->previous = vm->catch_frame;
      vm->catch_frame = catch_frame;
      break;
    }

    case OP_END_TRY: {
      b_catch_frame *catch_frame = vm->catch_frame->previous;
      free(vm->catch_frame);
      vm->catch_frame = catch_frame;
      break;
    }

    case OP_SWITCH: {
      b_obj_switch *sw = AS_SWITCH(READ_CONSTANT());
      b_value expr = peek(vm, 0);
      //      push(vm, OBJ_VAL(sw));

      b_value value;
      if (table_get(&sw->table, expr, &value)) {
        frame->ip += (int)AS_NUMBER(value);
      } else if (sw->default_ip != -1) {
        frame->ip += sw->default_ip;
      }
      //      pop_n(vm, 2);
      pop(vm);
      break;
    }

    case OP_CHOICE: {
      b_value _else = peek(vm, 0);
      b_value _then = peek(vm, 1);
      b_value _condition = peek(vm, 2);

      pop_n(vm, 3);
      if (!is_falsey(_condition)) {
        push(vm, _then);
      } else {
        push(vm, _else);
      }
      break;
    }

    default:
      break;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_LCONSTANT
#undef READ_STRING
#undef READ_LSTRING
#undef BINARY_OP
#undef BINARY_MOD_OP
}

b_ptr_result interpret(b_vm *vm, const char *source, const char *filename) {
  b_blob blob;
  init_blob(&blob);

  b_obj_func *function = compile(vm, source, filename, &blob);

  if (function == NULL) {
    free_blob(vm, &blob);
    return PTR_COMPILE_ERR;
  }

  push(vm, OBJ_VAL(function));
  call_function(vm, function, 0);

  b_ptr_result result = run(vm);

  return result;
}