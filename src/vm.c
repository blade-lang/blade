#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "config.h"
#include "memory.h"
#include "module.h"
#include "native.h"
#include "object.h"
#include "utf8.h"

#include "bytes.h"
#include "dict.h"
#include "file.h"
#include "list.h"
#include "bstring.h"
#include "range.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// for debugging...
#include "debug.h"

#define ERR_CANT_ASSIGN_EMPTY "empty cannot be assigned."

static inline void reset_stack(b_vm *vm) {
  vm->stack_top = vm->stack;
  vm->error_top = vm->errors;
  vm->frame_count = 0;
  vm->open_up_values = NULL;
}

static b_value get_stack_trace(b_vm *vm) {
  char *trace = calloc(0, sizeof(char));

  if (trace != NULL) {

    for (int i = vm->frame_count - 1; i >= 0; i--) {
      b_call_frame *frame = &vm->frames[i];
      b_obj_func *function = frame->closure->function;

      // -1 because the IP is sitting on the next instruction to be executed
      size_t instruction = frame->ip - function->blob.code - 1;
      int line = function->blob.lines[instruction];

      const char *trace_format = i != 0
          ? "    %s:%d -> %s()\n"
          : "    %s:%d -> %s()";
      char *fn_name = function->name == NULL ? "@.script": function->name->chars;
      size_t trace_line_length = snprintf(NULL, 0, trace_format, function->module->file, line, fn_name);

      char *trace_line = ALLOCATE(char, trace_line_length + 1);
      if (trace_line != NULL) {
        sprintf(trace_line, trace_format, function->module->file, line, fn_name);
        trace_line[(int) trace_line_length] = '\0';
      }

      trace = append_strings(trace, trace_line);
      free(trace_line);
    }

    return STRING_TT_VAL(trace);
  }

  return STRING_L_VAL("", 0);
}

bool propagate_exception(b_vm *vm, bool is_assert) {
  b_obj_instance *exception = AS_INSTANCE(peek(vm, 0));

  while (vm->frame_count > 0) {
    vm->current_frame = &vm->frames[vm->frame_count - 1];
    for (int i = vm->current_frame->handlers_count; i > 0; i--) {
      b_exception_frame handler = vm->current_frame->handlers[i - 1];
      b_obj_func *function = vm->current_frame->closure->function;

      if (handler.address != 0 && is_instance_of(exception->klass, handler.klass->name->chars)) {
        vm->current_frame->ip = &function->blob.code[handler.address];
        return true;
      } else if (handler.finally_address != 0) {
        push(vm, TRUE_VAL); // continue propagating once the 'finally' block completes
        vm->current_frame->ip = &function->blob.code[handler.finally_address];
        return true;
      }
    }

    vm->frame_count--;
  }

  fflush(stdout); // flush out anything on stdout first

  b_value message, trace;
  if(!is_assert) {
    fprintf(stderr, "Unhandled %s", exception->klass->name->chars);
  } else {
    fprintf(stderr, "Illegal State");
  }
  if (table_get(&exception->properties, STRING_L_VAL("message", 7), &message)) {
    char *error_message = value_to_string(vm, message)->chars;
    if(strlen(error_message) > 0) {
      fprintf(stderr, ": %s", error_message);
    } else {
      fprintf(stderr, ":");
    }
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "\n");
  }

  if (table_get(&exception->properties, STRING_L_VAL("stacktrace", 10), &trace)) {
    char *trace_str = value_to_string(vm, trace)->chars;
    fprintf(stderr, "  StackTrace:\n%s\n", trace_str);
  }

  return false;
}

bool print_exception(b_vm *vm, b_obj_instance *exception, bool is_assert) {
  if(vm->error_top - vm->errors > 0) {
    b_error_frame *error = peek_error(vm);
    error->value = OBJ_VAL(exception);
    return true;
  }

  fflush(stdout); // flush out anything on stdout first

  b_value message, trace;
  if(!is_assert) {
    fprintf(stderr, "Unhandled %s", exception->klass->name->chars);
  } else {
    fprintf(stderr, "Illegal State");
  }
  if (table_get(&exception->properties, STRING_L_VAL("message", 7), &message)) {
    char *error_message = value_to_string(vm, message)->chars;
    if(strlen(error_message) > 0) {
      fprintf(stderr, ": %s", error_message);
    } else {
      fprintf(stderr, ":");
    }
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "\n");
  }

  if (table_get(&exception->properties, STRING_L_VAL("stacktrace", 10), &trace)) {
    char *trace_str = value_to_string(vm, trace)->chars;
    fprintf(stderr, "  StackTrace:\n%s\n", trace_str);
  }

  return false;
}

bool push_exception_handler(b_vm *vm, b_obj_class *type, int address, int finally_address) {
  b_call_frame *frame = &vm->frames[vm->frame_count - 1];
  if (frame->handlers_count == MAX_EXCEPTION_HANDLERS) {
    do_runtime_error(vm, "too many nested exception handlers in one function");
    return false;
  }
  frame->handlers[frame->handlers_count].address = address;
  frame->handlers[frame->handlers_count].finally_address = finally_address;
  frame->handlers[frame->handlers_count].klass = type;
  frame->handlers_count++;
  return true;
}

bool do_throw_exception(b_vm *vm, bool is_assert, const char *format, ...) {

  va_list args;
  va_start(args, format);
  char *message = NULL;
  int length = vasprintf(&message, format, args);
  va_end(args);

  b_obj_instance *instance = create_exception(vm, take_string(vm, message, length));
  push(vm, OBJ_VAL(instance));

  b_value stacktrace = get_stack_trace(vm);
  push(vm, stacktrace);
  table_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
  table_set(vm, &instance->properties, STRING_L_VAL("type", 4),
    STRING_L_VAL(instance->klass->name->chars, instance->klass->name->length)
  );
  pop(vm);

  pop(vm); // pop the instance

  bool return_val = print_exception(vm, instance, is_assert);
  if(return_val) {
    // switch context immediately to the catch block
    b_error_frame *error = peek_error(vm);
    vm->frame_count -= vm->current_frame - error->frame;
    vm->current_frame = error->frame;
    vm->current_frame->ip = &error->frame->closure->function->blob.code[error->offset];
  }

  return return_val;
}

static void initialize_exceptions(b_vm *vm, b_obj_module *module) {
  b_obj_string *class_name = copy_string(vm, "Exception", 9);

  push(vm, OBJ_VAL(class_name));
  b_obj_class *klass = new_class(vm, class_name);
  pop(vm);

  push(vm, OBJ_VAL(klass));
  b_obj_func *function = new_function(vm, module, TYPE_METHOD);

  function->arity = 1;
  function->is_variadic = false;
  push(vm, OBJ_VAL(function));

  // g_loc 0
  write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
  write_blob(vm, &function->blob, (0 >> 8) & 0xff, 0);
  write_blob(vm, &function->blob, 0 & 0xff, 0);

  // g_loc 1
  write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
  write_blob(vm, &function->blob, (1 >> 8) & 0xff, 0);
  write_blob(vm, &function->blob, 1 & 0xff, 0);

  int message_const = add_constant(vm, &function->blob, STRING_L_VAL("message", 7));

  // s_prop 0
  write_blob(vm, &function->blob, OP_SET_PROPERTY, 0);
  write_blob(vm, &function->blob, (message_const >> 8) & 0xff, 0);
  write_blob(vm, &function->blob, message_const & 0xff, 0);

  // pop
  write_blob(vm, &function->blob, OP_POP, 0);
  write_blob(vm, &function->blob, OP_POP, 0);

  // g_loc 0
//  write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
//  write_blob(vm, &function->blob, (0 >> 8) & 0xff, 0);
//  write_blob(vm, &function->blob, 0 & 0xff, 0);

  // ret
  write_blob(vm, &function->blob, OP_RETURN, 0);

  b_obj_closure *closure = new_closure(vm, function);
  pop(vm);

  // set class constructor
  push(vm, OBJ_VAL(closure));
  table_set(vm, &klass->methods, OBJ_VAL(class_name), OBJ_VAL(closure));
  klass->initializer = OBJ_VAL(closure);

  // set class properties
  table_set(vm, &klass->properties, STRING_L_VAL("message", 7), NIL_VAL);
  table_set(vm, &klass->properties, STRING_L_VAL("stacktrace", 10), NIL_VAL);

  table_set(vm, &vm->globals, OBJ_VAL(class_name), OBJ_VAL(klass));
  pop(vm); // for class

  pop(vm);
  pop(vm); // assert error name

  vm->exception_class = klass;
}

inline b_obj_instance *create_exception(b_vm *vm, b_obj_string *message) {
  b_obj_instance *instance = new_instance(vm, vm->exception_class);
  push(vm, OBJ_VAL(instance));
  table_set(vm, &instance->properties, STRING_L_VAL("message", 7), OBJ_VAL(message));
  pop(vm);
  return instance;
}

void do_runtime_error(b_vm *vm, const char *format, ...) {
  fflush(stdout); // flush out anything on stdout first

  b_call_frame *frame = &vm->frames[vm->frame_count - 1];
  b_obj_func *function = frame->closure->function;

  size_t instruction = frame->ip - function->blob.code - 1;
  int line = function->blob.lines[instruction];

  fprintf(stderr, "RuntimeError: ");

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, " -> %s:%d ", function->module->file, line);
  fputs("\n", stderr);

  if (vm->frame_count > 1) {
    fprintf(stderr, "StackTrace:\n");
    for (int i = vm->frame_count - 1; i >= 0; i--) {
      frame = &vm->frames[i];
      function = frame->closure->function;

      // -1 because the IP is sitting on the next instruction to be executed
      instruction = frame->ip - function->blob.code - 1;

      fprintf(stderr, "    %s:%d -> ", function->module->file, function->blob.lines[instruction]);
      if (function->name == NULL) {
        fprintf(stderr, "@.script()");
      } else {
        fprintf(stderr, "%s()", function->name->chars);
      }
      fprintf(stderr, "\n");
    }
  }

  reset_stack(vm);
}

static inline void grow_vm_stack(b_vm *vm, size_t new_capacity) {
  b_value *new_stack = ALLOCATE(b_value, new_capacity);
  for(size_t i = 0; i < new_capacity; i++) {
    new_stack[i] = EMPTY_VAL;
  }

  size_t old_capacity = vm->stack_capacity;
  for (size_t i = 0; i < vm->stack_capacity; i++) {
    new_stack[i] = vm->stack[i];
  }

  FREE_ARRAY(b_value, vm->stack, vm->stack_capacity);

  vm->stack = new_stack;
  vm->stack_top = vm->stack + old_capacity;
  vm->stack_capacity = new_capacity;
}

inline void push(b_vm *vm, b_value value) {
  if(vm->stack_top - vm->stack == vm->stack_capacity) {
    size_t capacity = GROW_CAPACITY(vm->stack_capacity);
    grow_vm_stack(vm, capacity);
  }

  *vm->stack_top = value;
  vm->stack_top++;
}

inline void push_error(b_vm *vm, b_error_frame *frame) {
  if(vm->error_top - vm->errors > ERRORS_MAX) {
    fprintf(stderr, "Exit: Maximum open catch blocks %d exceeded.\n", ERRORS_MAX);
    exit(EXIT_RUNTIME);
  }

  *vm->error_top = frame;
  vm->error_top++;
}

inline b_error_frame* pop_error(b_vm *vm) {
  vm->error_top--;
  return *vm->error_top;
}

inline b_error_frame* peek_error(b_vm *vm) {
  return vm->error_top[-1];
}

inline b_value pop(b_vm *vm) {
  if(vm->stack_top == vm->stack) {
    fprintf(stderr, "Exit: Stack integrity check at end of stack failed.\n");
    exit(EXIT_TERMINAL);
  }

  vm->stack_top--;
  return *vm->stack_top;
}

inline b_value pop_n(b_vm *vm, int n) {
  if(vm->stack_top - vm->stack < n) {
    fprintf(stderr, "Exit: Stack integrity check at %ld failed.\n", vm->stack_top - vm->stack);
    exit(EXIT_TERMINAL);
  }

  vm->stack_top -= n;
  return *vm->stack_top;
}

inline b_value peek(b_vm *vm, int distance) {
  if(vm->stack_top - vm->stack < distance + 1) {
    fprintf(stderr, "Exit: Stack integrity check at distance %d failed.\n", distance);
    exit(EXIT_TERMINAL);
  }

  return vm->stack_top[-1 - distance]; 
}

static inline void define_native(b_vm *vm, const char *name, b_native_fn function) {
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
  DEFINE_NATIVE(is_bytes);
  DEFINE_NATIVE(is_file);
  DEFINE_NATIVE(is_iterable);
  DEFINE_NATIVE(instance_of);
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
#define DEFINE_RANGE_METHOD(name) DEFINE_METHOD(range, name)

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
  DEFINE_STRING_METHOD(replace_with);
  DEFINE_STRING_METHOD(ascii);
  DEFINE_STRING_METHOD(each);
  DEFINE_STRING_METHOD(case_fold);
  define_native_method(vm, &vm->methods_string, "@iter", native_method_string__iter__);
  define_native_method(vm, &vm->methods_string, "@itern", native_method_string__itern__);

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
  DEFINE_LIST_METHOD(zip_from);
  DEFINE_LIST_METHOD(to_dict);
  DEFINE_LIST_METHOD(each);
  DEFINE_LIST_METHOD(map);
  DEFINE_LIST_METHOD(filter);
  DEFINE_LIST_METHOD(some);
  DEFINE_LIST_METHOD(every);
  DEFINE_LIST_METHOD(reduce);
  define_native_method(vm, &vm->methods_list, "@iter", native_method_list__iter__);
  define_native_method(vm, &vm->methods_list, "@itern", native_method_list__itern__);

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
  DEFINE_DICT_METHOD(is_empty);
  DEFINE_DICT_METHOD(find_key);
  DEFINE_DICT_METHOD(to_list);
  DEFINE_DICT_METHOD(each);
  DEFINE_DICT_METHOD(filter);
  DEFINE_DICT_METHOD(some);
  DEFINE_DICT_METHOD(every);
  DEFINE_DICT_METHOD(reduce);
  define_native_method(vm, &vm->methods_dict, "@iter", native_method_dict__iter__);
  define_native_method(vm, &vm->methods_dict, "@itern", native_method_dict__itern__);

  // file methods
  DEFINE_FILE_METHOD(exists);
  DEFINE_FILE_METHOD(close);
  DEFINE_FILE_METHOD(open);
  DEFINE_FILE_METHOD(read);
  DEFINE_FILE_METHOD(gets);
  DEFINE_FILE_METHOD(write);
  DEFINE_FILE_METHOD(puts);
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
  DEFINE_BYTES_METHOD(index_of);
  DEFINE_BYTES_METHOD(pop);
  DEFINE_BYTES_METHOD(remove);
  DEFINE_BYTES_METHOD(reverse);
  DEFINE_BYTES_METHOD(first);
  DEFINE_BYTES_METHOD(last);
  DEFINE_BYTES_METHOD(get);
  DEFINE_BYTES_METHOD(split);
  DEFINE_BYTES_METHOD(dispose);
  DEFINE_BYTES_METHOD(is_alpha);
  DEFINE_BYTES_METHOD(is_alnum);
  DEFINE_BYTES_METHOD(is_number);
  DEFINE_BYTES_METHOD(is_lower);
  DEFINE_BYTES_METHOD(is_upper);
  DEFINE_BYTES_METHOD(is_space);
  DEFINE_BYTES_METHOD(to_list);
  DEFINE_BYTES_METHOD(to_string);
  DEFINE_BYTES_METHOD(each);
  define_native_method(vm, &vm->methods_bytes, "@iter", native_method_bytes__iter__);
  define_native_method(vm, &vm->methods_bytes, "@itern", native_method_bytes__itern__);
  // range
  DEFINE_RANGE_METHOD(lower);
  DEFINE_RANGE_METHOD(upper);
  DEFINE_RANGE_METHOD(range);
  DEFINE_RANGE_METHOD(loop);
  define_native_method(vm, &vm->methods_range, "@iter", native_method_range__iter__);
  define_native_method(vm, &vm->methods_range, "@itern", native_method_range__itern__);

#undef DEFINE_STRING_METHOD
#undef DEFINE_LIST_METHOD
#undef DEFINE_DICT_METHOD
#undef DEFINE_FILE_METHOD
#undef DEFINE_BYTES_METHOD
#undef DEFINE_RANGE_METHOD
}

void init_vm(b_vm *vm) {

  vm->stack = ALLOCATE(b_value, STACK_MIN);
  vm->stack_capacity = STACK_MIN;

  reset_stack(vm);
  vm->compiler = NULL;
  vm->objects = NULL;
  vm->exception_class = NULL;
  vm->current_frame = NULL;
  vm->root_file = NULL;
  vm->bytes_allocated = 0;
  vm->gc_protected = 0;
  vm->next_gc = DEFAULT_GC_START; // default is 1mb. Can be modified via the -g flag.
  vm->is_repl = false;
  vm->mark_value = true;
  vm->show_warnings = false;
  vm->should_print_bytecode = false;
  vm->should_exit_after_bytecode = false;

  vm->gray_count = 0;
  vm->gray_capacity = 0;
  vm->gray_stack = NULL;

  vm->std_args = NULL;
  vm->std_args_count = 0;

  init_table(&vm->modules);
  init_table(&vm->strings);
  init_table(&vm->globals);

  // object methods tables
  init_table(&vm->methods_string);
  init_table(&vm->methods_list);
  init_table(&vm->methods_dict);
  init_table(&vm->methods_file);
  init_table(&vm->methods_bytes);
  init_table(&vm->methods_range);

  init_builtin_functions(vm);
  init_builtin_methods(vm);
}

void free_vm(b_vm *vm) {
  free_objects(vm);

  // since object in module can exist in globals
  // it must come before
  free_table(vm, &vm->modules);
  free_table(vm, &vm->globals);
  free_table(vm, &vm->strings);

  free_table(vm, &vm->methods_string);
  free_table(vm, &vm->methods_list);
  free_table(vm, &vm->methods_dict);
  free_table(vm, &vm->methods_file);
  free_table(vm, &vm->methods_bytes);

  free(vm->stack);
  free(vm);
}

static inline bool is_private(b_obj_string *name) {
  return name->length > 0 && name->chars[0] == '_';
}

static bool call(b_vm *vm, b_obj_closure *closure, int arg_count) {
  // fill empty parameters if not variadic
  for (; !closure->function->is_variadic && arg_count < closure->function->arity; arg_count++) {
    push(vm, NIL_VAL);
  }

  // handle variadic arguments...
  if (closure->function->is_variadic && arg_count >= closure->function->arity - 1) {
    int va_args_start = arg_count - closure->function->arity;
    b_obj_list *args_list = new_list(vm);
    push(vm, OBJ_VAL(args_list));

    for (int i = va_args_start; i >= 0; i--) {
      write_value_arr(vm, &args_list->items, peek(vm, i + 1));
    }
    arg_count -= va_args_start;
    pop_n(vm, va_args_start + 2); // +1 for the gc protection push above
    push(vm, OBJ_VAL(args_list));
  }

  if (arg_count != closure->function->arity) {
    pop_n(vm, arg_count);
    if (closure->function->is_variadic) {
      return throw_exception(vm, "expected at least %d arguments but got %d",
                             closure->function->arity - 1, arg_count);
    } else {
      return throw_exception(vm, "expected %d arguments but got %d",
                             closure->function->arity, arg_count);
    }
  }

  if (vm->frame_count == FRAMES_MAX) {
    pop_n(vm, arg_count);
    return throw_exception(vm, "stack overflow");
  }

  b_call_frame *frame = &vm->frames[vm->frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->blob.code;

  frame->slots = vm->stack_top - arg_count - 1;
  return true;
}

static inline bool call_native_method(b_vm *vm, b_obj_native *native, int arg_count) {
  if (native->function(vm, arg_count, vm->stack_top - arg_count)) {
    CLEAR_GC();
    pop_n(vm, arg_count);
  } else {
    CLEAR_GC();
  }
  return true;
}

bool call_value(b_vm *vm, b_value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        b_obj_bound *bound = AS_BOUND(callee);
        vm->stack_top[-arg_count - 1] = bound->receiver;
        return call(vm, bound->method, arg_count);
      }

      case OBJ_CLASS: {
        b_obj_class *klass = AS_CLASS(callee);
        vm->stack_top[-arg_count - 1] = OBJ_VAL(new_instance(vm, klass));
        if (!IS_EMPTY(klass->initializer)) {
          return call(vm, AS_CLOSURE(klass->initializer), arg_count);
        } else if (klass->superclass != NULL && !IS_EMPTY(klass->superclass->initializer)) {
          return call(vm, AS_CLOSURE(klass->superclass->initializer), arg_count);
        } else if (arg_count != 0) {
          return throw_exception(vm, "%s constructor expects 0 arguments, %d given", klass->name->chars, arg_count);
        }
        return true;
      }

      case OBJ_MODULE: {
        b_obj_module *module = AS_MODULE(callee);
        b_value callable;
        if(table_get(&module->values, STRING_VAL(module->name), &callable)) {
          return call_value(vm, callable, arg_count);
        }

        return throw_exception(vm, "module %s does not export a default function", module->name);
      }

      case OBJ_CLOSURE: {
        return call(vm, AS_CLOSURE(callee), arg_count);
      }

      case OBJ_NATIVE: {
        return call_native_method(vm, AS_NATIVE(callee), arg_count);
      }

      default: // non callable
        break;
    }
  }

  return throw_exception(vm, "object of type %s is not callable", value_type(callee));
}

static inline b_func_type get_method_type(b_value method) {
  switch (OBJ_TYPE(method)) {
    case OBJ_NATIVE: return AS_NATIVE(method)->type;
    case OBJ_CLOSURE: return AS_CLOSURE(method)->function->type;
    default: return TYPE_FUNCTION;
  }
}

inline bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name,
                       int arg_count) {
  b_value method;
  if (table_get(&klass->methods, OBJ_VAL(name), &method)) {
    if (get_method_type(method) == TYPE_PRIVATE) {
      return throw_exception(vm, "cannot call private method '%s' from instance of %s",
                             name->chars, klass->name->chars);
    }

    return call_value(vm, method, arg_count);
  }

  return throw_exception(vm, "undefined method '%s' in %s", name->chars, klass->name->chars);
}

static bool invoke_self(b_vm *vm, b_obj_string *name, int arg_count) {
  b_value receiver = peek(vm, arg_count);
  b_value value;

  if (IS_INSTANCE(receiver)) {
    b_obj_instance *instance = AS_INSTANCE(receiver);

    if (table_get(&instance->klass->methods, OBJ_VAL(name), &value)) {
      return call_value(vm, value, arg_count);
    }

    if (table_get(&instance->properties, OBJ_VAL(name), &value)) {
      vm->stack_top[-arg_count - 1] = value;
      return call_value(vm, value, arg_count);
    }
  } else if (IS_CLASS(receiver)) {
    if (table_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value)) {
      if (get_method_type(value) == TYPE_STATIC) {
        return call_value(vm, value, arg_count);
      }

      return throw_exception(vm, "cannot call non-static method %s() on non instance", name->chars);
    }
  }

  return throw_exception(vm, "cannot call method %s on object of type %s",
                         name->chars, value_type(receiver));
}

static bool invoke(b_vm *vm, b_obj_string *name, int arg_count) {
  b_value receiver = peek(vm, arg_count);
  b_value value;

  if (!IS_OBJ(receiver)) {
    // @TODO: have methods for non objects as well.
    return throw_exception(vm, "non-object %s has no method", value_type(receiver));
  } else {
    switch (AS_OBJ(receiver)->type) {
      case OBJ_MODULE: {
        b_obj_module *module = AS_MODULE(receiver);
        if (table_get(&module->values, OBJ_VAL(name), &value)) {
          if (is_private(name)) {
            return throw_exception(vm, "cannot call private module method '%s'", name->chars);
          }
          return call_value(vm, value, arg_count);
        }
        return throw_exception(vm, "module %s does not define class or method %s()", module->name, name->chars);
        break;
      }
      case OBJ_CLASS: {
        if (table_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value)) {
          if (get_method_type(value) == TYPE_PRIVATE) {
            return throw_exception(vm, "cannot call private method %s() on %s",
                                   name->chars, AS_CLASS(receiver)->name->chars);
          }
          return call_value(vm, value, arg_count);
        } else if (table_get(&AS_CLASS(receiver)->static_properties, OBJ_VAL(name), &value)) {
          return call_value(vm, value, arg_count);
        }

        return throw_exception(vm, "unknown method %s() in class %s", name->chars, AS_CLASS(receiver)->name->chars);
      }
      case OBJ_INSTANCE: {
        b_obj_instance *instance = AS_INSTANCE(receiver);

        if (table_get(&instance->properties, OBJ_VAL(name), &value)) {
          vm->stack_top[-arg_count - 1] = value;
          return call_value(vm, value, arg_count);
        }

        return invoke_from_class(vm, instance->klass, name, arg_count);
      }
      case OBJ_STRING: {
        if (table_get(&vm->methods_string, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }
        return throw_exception(vm, "String has no method %s()", name->chars);
      }
      case OBJ_LIST: {
        if (table_get(&vm->methods_list, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }
        return throw_exception(vm, "List has no method %s()", name->chars);
      }
      case OBJ_RANGE: {
        if (table_get(&vm->methods_range, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }
        return throw_exception(vm, "Range has no method %s()", name->chars);
      }
      case OBJ_DICT: {
        if (table_get(&vm->methods_dict, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }

        // NEW in v0.0.84, dictionaries can declare extra methods as part of their entries.
        else if(table_get(&AS_DICT(receiver)->items, OBJ_VAL(name), &value)) {
          if(IS_CLOSURE(value)) {
            return call_value(vm, value, arg_count);
          }
        }
        return throw_exception(vm, "Dict has no method %s()", name->chars);
      }
      case OBJ_FILE: {
        if (table_get(&vm->methods_file, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }
        return throw_exception(vm, "File has no method %s()", name->chars);
      }
      case OBJ_BYTES: {
        if (table_get(&vm->methods_bytes, OBJ_VAL(name), &value)) {
          return call_native_method(vm, AS_NATIVE(value), arg_count);
        }
        return throw_exception(vm, "Bytes has no method %s()", name->chars);
      }
      default: {
        return throw_exception(vm, "cannot call method %s on object of type %s",
                               name->chars, value_type(receiver));
      }
    }
  }
}

static inline bool bind_method(b_vm *vm, b_obj_class *klass, b_obj_string *name) {
  b_value method;
  if (table_get(&klass->methods, OBJ_VAL(name), &method)) {
    if (get_method_type(method) == TYPE_PRIVATE) {
      return throw_exception(vm, "cannot get private property '%s' from instance", name->chars);
    }

    b_obj_bound *bound = new_bound_method(vm, peek(vm, 0), AS_CLOSURE(method));
    pop(vm);
    push(vm, OBJ_VAL(bound));
    return true;
  }

  return throw_exception(vm, "undefined property '%s'", name->chars);
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

static inline void close_up_values(b_vm *vm, const b_value *last) {
  while (vm->open_up_values != NULL && vm->open_up_values->location >= last) {
    b_obj_up_value *up_value = vm->open_up_values;
    up_value->closed = *up_value->location;
    up_value->location = &up_value->closed;
    vm->open_up_values = up_value->next;
  }
}

static inline void define_method(b_vm *vm, b_obj_string *name) {
  b_value method = peek(vm, 0);
  b_obj_class *klass = AS_CLASS(peek(vm, 1));

  table_set(vm, &klass->methods, OBJ_VAL(name), method);
  if (get_method_type(method) == TYPE_INITIALIZER) {
    klass->initializer = method;
  }
  pop(vm);
}

static inline void define_property(b_vm *vm, b_obj_string *name, bool is_static) {
  b_value property = peek(vm, 0);
  b_obj_class *klass = AS_CLASS(peek(vm, 1));

  if (!is_static) {
    table_set(vm, &klass->properties, OBJ_VAL(name), property);
  } else {
    table_set(vm, &klass->static_properties, OBJ_VAL(name), property);
  }
  pop(vm);
}

inline bool is_false(b_value value) {
  if (IS_BOOL(value))
    return IS_BOOL(value) && !AS_BOOL(value);
  if (IS_NIL(value) || IS_EMPTY(value))
    return true;

  // -1 is the number equivalent of false in Blade
  if (IS_NUMBER(value))
    return AS_NUMBER(value) < 0;

  // Non-empty strings are true, empty strings are false.
  if (IS_STRING(value))
    return AS_STRING(value)->length < 1;

  // Non-empty bytes are true, empty strings are false.
  if (IS_BYTES(value))
    return AS_BYTES(value)->bytes.count < 1;

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
    if ((int) strlen(klass2_name) == klass1->name->length
        && memcmp(klass1->name->chars, klass2_name, klass1->name->length) == 0) {
      return true;
    }
    klass1 = klass1->superclass;
  }

  return false;
}

inline bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value) {
  b_value temp_value;
  if (!table_get(&dict->items, key, &temp_value)) {
    write_value_arr(vm, &dict->names, key); // add key if it doesn't exist.
  }
  return table_set(vm, &dict->items, key, value);
}

inline void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value) {
  dict_set_entry(vm, dict, key, value);
}

inline bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value) {
  return table_get(&dict->items, key, value);
}

static b_obj_string *multiply_string(b_vm *vm, b_obj_string *str, double number) {
  int times = (int) number;

  if (times <= 0) // 'str' * 0 == '', 'str' * -1 == ''
    return copy_string(vm, "", 0);
  else if (times == 1) // 'str' * 1 == 'str'
    return str;

  int total_length = str->length * times;
  char *result = ALLOCATE(char, (size_t) total_length + 1);

  for (int i = 0; i < times; i++) {
    memcpy(result + (str->length * i), str->chars, str->length);
  }
  result[total_length] = '\0';
  return take_string(vm, result, total_length);
}

static b_obj_list *add_list(b_vm *vm, b_obj_list *a, b_obj_list *b) {
  b_obj_list *list = new_list(vm);
  push(vm, OBJ_VAL(list));

  for (int i = 0; i < a->items.count; i++) {
    write_value_arr(vm, &list->items, a->items.values[i]);
  }

  for (int i = 0; i < b->items.count; i++) {
    write_value_arr(vm, &list->items, b->items.values[i]);
  }

  pop(vm);
  return list;
}

static inline b_obj_bytes *add_bytes(b_vm *vm, b_obj_bytes *a, b_obj_bytes *b) {
  b_obj_bytes *bytes = new_bytes(vm, a->bytes.count + b->bytes.count);

  memcpy(bytes->bytes.bytes, a->bytes.bytes, a->bytes.count);
  memcpy(bytes->bytes.bytes + a->bytes.count, b->bytes.bytes, b->bytes.count);

  return bytes;
}

static inline void multiply_list(b_vm *vm, b_obj_list *a, b_obj_list *new_list, int times) {
  for (int i = 0; i < times; i++) {
    for (int j = 0; j < a->items.count; j++) {
      write_value_arr(vm, &new_list->items, a->items.values[j]);
    }
  }
}

static bool dict_get_index(b_vm *vm, b_obj_dict *dict, bool will_assign) {
  b_value index = peek(vm, 0);

  b_value result;
  if (dict_get_entry(dict, index, &result)) {
    if (!will_assign) {
      pop_n(vm, 2); // we can safely get rid of the index from the stack
    }
    push(vm, result);
    return true;
  }

  pop_n(vm, 1);
  return throw_exception(vm, "invalid index %s", value_to_string(vm, index)->chars);
}

static bool module_get_index(b_vm *vm, b_obj_module *module, bool will_assign) {
  b_value index = peek(vm, 0);

  b_value result;
  if (table_get(&module->values, index, &result)) {
    if (!will_assign) {
      pop_n(vm, 2); // we can safely get rid of the index from the stack
    }
    push(vm, result);
    return true;
  }

  pop_n(vm, 1);
  return throw_exception(vm, "%s is undefined in module %s", value_to_string(vm, index)->chars, module->name);
}

static bool string_get_index(b_vm *vm, b_obj_string *string, bool will_assign) {
  b_value lower = peek(vm, 0);

  if (!IS_NUMBER(lower)) {
    pop_n(vm, 1);
    return throw_exception(vm, "strings are numerically indexed");
  }

  int index = AS_NUMBER(lower);
  int length = string->is_ascii ? string->length : string->utf8_length;
  int real_index = index;
  if (index < 0)
    index = length + index;

  if (index < length && index >= 0) {

    int start = index, end = index + 1;
    if(!string->is_ascii){
      utf8slice(string->chars, &start, &end);
    }

    if (!will_assign) {
      // we can safely get rid of the index from the stack
      pop_n(vm, 2); // +1 for the string itself
    }

    push(vm, STRING_L_VAL(string->chars + start, end - start));
    return true;
  } else {
    pop_n(vm, 1);
    return throw_exception(vm, "string index %d out of range", real_index);
  }
}

static bool string_get_ranged_index(b_vm *vm, b_obj_string *string, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper))) {
    pop_n(vm, 2);
    return throw_exception(vm, "string are numerically indexed");
  }
  int length = string->is_ascii ? string->length : string->utf8_length;

  int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
  int upper_index = IS_NIL(upper) ? length : AS_NUMBER(upper);

  if (lower_index < 0 || (upper_index < 0 && ((length + upper_index) < 0)) || lower_index >= length) {
    // always return an empty string...
    if (!will_assign) {
      pop_n(vm, 3); // +1 for the string itself
    }
    push(vm, STRING_L_VAL("", 0));
    return true;
  }

  if (upper_index < 0)
    upper_index = length + upper_index;

  if (upper_index > length)
    upper_index = length;

  int start = lower_index, end = upper_index;
  if(!string->is_ascii) {
    utf8slice(string->chars, &start, &end);
  }

  if (!will_assign) {
    pop_n(vm, 3); // +1 for the string itself
  }

  push(vm, STRING_L_VAL(string->chars + start, end - start));
  return true;
}

static bool bytes_get_index(b_vm *vm, b_obj_bytes *bytes, bool will_assign) {
  b_value lower = peek(vm, 0);

  if (!IS_NUMBER(lower)) {
    pop_n(vm, 1);
    return throw_exception(vm, "bytes are numerically indexed");
  }

  int index = AS_NUMBER(lower);
  int real_index = index;
  if (index < 0)
    index = bytes->bytes.count + index;

  if (index < bytes->bytes.count && index >= 0) {
    if (!will_assign) {
      // we can safely get rid of the index from the stack
      pop_n(vm, 2); // +1 for the bytes itself
    }

    push(vm, NUMBER_VAL((int) bytes->bytes.bytes[index]));
    return true;
  } else {
    pop_n(vm, 1);
    return throw_exception(vm, "bytes index %d out of range", real_index);
  }
}

static bool bytes_get_ranged_index(b_vm *vm, b_obj_bytes *bytes, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper))) {
    pop_n(vm, 2);
    return throw_exception(vm, "bytes are numerically indexed");
  }

  int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
  int upper_index = IS_NIL(upper) ? bytes->bytes.count : AS_NUMBER(upper);

  if (lower_index < 0 ||
      (upper_index < 0 && ((bytes->bytes.count + upper_index) < 0)) || lower_index >= bytes->bytes.count) {
    // always return an empty bytes...
    if (!will_assign) {
      pop_n(vm, 3); // +1 for the bytes itself
    }
    push(vm, OBJ_VAL(new_bytes(vm, 0)));
    return true;
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

static bool list_get_index(b_vm *vm, b_obj_list *list, bool will_assign) {
  b_value lower = peek(vm, 0);

  if (!IS_NUMBER(lower)) {
    pop(vm);
    return throw_exception(vm, "list are numerically indexed");
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
    pop(vm);
    return throw_exception(vm, "list index %d out of range", real_index);
  }
}

static bool list_get_ranged_index(b_vm *vm, b_obj_list *list, bool will_assign) {
  b_value upper = peek(vm, 0);
  b_value lower = peek(vm, 1);

  if (!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper))) {
    pop_n(vm, 2);
    return throw_exception(vm, "list are numerically indexed");
  }

  int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
  int upper_index = IS_NIL(upper) ? list->items.count : AS_NUMBER(upper);

  if (lower_index < 0 || (upper_index < 0 && ((list->items.count + upper_index) < 0)) || lower_index >= list->items.count) {
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
  push(vm, OBJ_VAL(n_list)); // gc protect

  for (int i = lower_index; i < upper_index; i++) {
    write_value_arr(vm, &n_list->items, list->items.values[i]);
  }
  pop(vm);  // clear gc protect

  if (!will_assign) {
    pop_n(vm, 3); // +1 for the list itself
  }
  push(vm, OBJ_VAL(n_list));
  return true;
}

static inline void dict_set_index(b_vm *vm, b_obj_dict *dict, b_value index, b_value value) {
  dict_set_entry(vm, dict, index, value);
  pop_n(vm, 3); // pop the value, index and dict out

  // leave the value on the stack for consumption
  // e.g. variable = dict[index] = 10
  push(vm, value);
}

static inline void module_set_index(b_vm *vm, b_obj_module *module, b_value index, b_value value) {
  table_set(vm, &module->values, index, value);
  pop_n(vm, 3); // pop the value, index and dict out

  // leave the value on the stack for consumption
  // e.g. variable = dict[index] = 10
  push(vm, value);
}

static bool list_set_index(b_vm *vm, b_obj_list *list, b_value index, b_value value) {
  if (!IS_NUMBER(index)) {
    pop_n(vm, 3); // pop the value, index and list out
    return throw_exception(vm, "list are numerically indexed");
  }

  int _position = AS_NUMBER(index);
  int position = _position < 0 ? list->items.count + _position : _position;

  if (position < list->items.count && position > -(list->items.count)) {
    list->items.values[position] = value;
    pop_n(vm, 3); // pop the value, index and list out

    // leave the value on the stack for consumption
    // e.g. variable = list[index] = 10
    push(vm, value);
    return true;
  }

  pop_n(vm, 3); // pop the value, index and list out
  return throw_exception(vm, "lists index %d out of range", _position);
}

static bool bytes_set_index(b_vm *vm, b_obj_bytes *bytes, b_value index, b_value value) {
  if (!IS_NUMBER(index)) {
    pop_n(vm, 3); // pop the value, index and bytes out
    return throw_exception(vm, "bytes are numerically indexed");
  } else if (!IS_NUMBER(value) || AS_NUMBER(value) < 0 || AS_NUMBER(value) > 255) {
    pop_n(vm, 3); // pop the value, index and bytes out
    return throw_exception(vm, "invalid byte. bytes are numbers between 0 and 255.");
  }

  int _position = AS_NUMBER(index);
  int byte = AS_NUMBER(value);

  int position = _position < 0 ? bytes->bytes.count + _position : _position;

  if (position < bytes->bytes.count && position > -(bytes->bytes.count)) {
    bytes->bytes.bytes[position] = (unsigned char) byte;
    pop_n(vm, 3); // pop the value, index and bytes out

    // leave the value on the stack for consumption
    // e.g. variable = bytes[index] = 10
    push(vm, value);
    return true;
  }

  pop_n(vm, 3); // pop the value, index and bytes out
  return throw_exception(vm, "bytes index %d out of range", _position);
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
    char *chars = ALLOCATE(char, (size_t) length + 1);
    memcpy(chars, num_str, num_length);
    memcpy(chars + num_length, b->chars, b->length);
    chars[length] = '\0';

    int utf8len = utf8length(chars);
    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = utf8len;

    pop_n(vm, 2);
    push(vm, OBJ_VAL(result));
  } else if (IS_NUMBER(_b)) {
    b_obj_string *a = AS_STRING(_a);
    double b = AS_NUMBER(_b);

    char num_str[27]; // + 1 for null terminator
    int num_length = sprintf(num_str, NUMBER_FORMAT, b);

    int length = num_length + a->length;
    char *chars = ALLOCATE(char, (size_t) length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, num_str, num_length);
    chars[length] = '\0';

    int utf8len = utf8length(chars);
    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = utf8len;

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

    // int new_utf8_length = utf8length(chars);
    b_obj_string *result = take_string(vm, chars, length);
    result->utf8_length = a->utf8_length + b->utf8_length;
    // result->utf8_length = new_utf8_length;

    pop_n(vm, 2);
    push(vm, OBJ_VAL(result));
  } else {
    return false;
  }

  return true;
}

static inline int floor_div(double a, double b) {
  int d = (int) a / (int) b;
  return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

static inline double modulo(double a, double b) {
  double r = fmod(a, b);
  if (r!=0 && ((r<0) != (b<0))) {
    r += b;
  }
  return r;
}

b_ptr_result run(b_vm *vm, int exit_frame) {
  vm->current_frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*vm->current_frame->ip++)

#define READ_SHORT()                                                           \
  (vm->current_frame->ip += 2, (uint16_t)((vm->current_frame->ip[-2] << 8) | vm->current_frame->ip[-1]))

#define READ_CONSTANT()                                                        \
  (vm->current_frame->closure->function->blob.constants.values[READ_SHORT()])

#define READ_STRING() (AS_STRING(READ_CONSTANT()))

#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      runtime_error("unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
                     break;        \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(a op b));                                                    \
  } while (false)

#define BINARY_BIT_OP(op)                                                \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      runtime_error("unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
                     break;       \
    }                                                                          \
    long b = AS_NUMBER(pop(vm));                                       \
    long a = AS_NUMBER(pop(vm));                        \
    push(vm, INTEGER_VAL(a op b));                                          \
  } while (false)

#define BINARY_MOD_OP(type, op)                                                \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      runtime_error("unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
                     break;        \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(op(a, b)));                                                  \
  } while (false)

  for (;;) {
    // try...finally... (i.e. try without a catch but finally
    // whose try body raises an exception)
    // can cause us to go into an invalid mode where frame count == 0
    // to fix this, we need to exit with an appropriate mode here.
    if (vm->frame_count == 0) {
      return PTR_RUNTIME_ERR;
    }

#if defined(DEBUG_STACK) && DEBUG_STACK
      printf("          ");
      for (b_value *slot = vm->stack; slot < vm->stack_top; slot++) {
        printf("[ ");
        print_value(*slot);
        printf(" ]");
      }
      printf("\n");
      disassemble_instruction(
          &vm->current_frame->closure->function->blob,
          (int) (vm->current_frame->ip - vm->current_frame->closure->function->blob.code));
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
            runtime_error("unsupported operand + for %s and %s", value_type(peek(vm, 0)), value_type(peek(vm, 1)));
            break;
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
          int number = (int) AS_NUMBER(pop(vm));
          b_obj_list *list = AS_LIST(peek(vm, 0));
          b_obj_list *n_list = new_list(vm);
          push(vm, OBJ_VAL(n_list));
          multiply_list(vm, list, n_list, number);
          pop_n(vm, 2);
          push(vm, OBJ_VAL(n_list));
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
        BINARY_MOD_OP(NUMBER_VAL, modulo);
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
          runtime_error("operator - not defined for object of type %s", value_type(peek(vm, 0)));
          break;
        }
        push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
        break;
      }
      case OP_BIT_NOT: {
        if (!IS_NUMBER(peek(vm, 0))) {
          runtime_error("operator ~ not defined for object of type %s", value_type(peek(vm, 0)));
          break;
        }
        push(vm, INTEGER_VAL(~((int) AS_NUMBER(pop(vm)))));
        break;
      }
      case OP_AND: {
        BINARY_BIT_OP(&);
        break;
      }
      case OP_OR: {
        BINARY_BIT_OP(|);
        break;
      }
      case OP_XOR: {
        BINARY_BIT_OP(^);
        break;
      }
      case OP_LSHIFT: {
        BINARY_BIT_OP(<<);
        break;
      }
      case OP_RSHIFT: {
        BINARY_BIT_OP(>>);
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
        push(vm, BOOL_VAL(is_false(pop(vm))));
        break;
      case OP_NIL:
        push(vm, NIL_VAL);
        break;
      case OP_EMPTY:
        push(vm, EMPTY_VAL);
        break;
      case OP_TRUE:
        push(vm, BOOL_VAL(true));
        break;
      case OP_FALSE:
        push(vm, BOOL_VAL(false));
        break;

      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        vm->current_frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_false(peek(vm, 0))) {
          vm->current_frame->ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        vm->current_frame->ip -= offset;
        break;
      }

      case OP_ECHO: {
        b_value val = peek(vm, 0);
        if (vm->is_repl) {
          echo_value(val);
        } else {
          print_value(val);
        }
        if(!IS_EMPTY(val)) {
          printf("\n");
        }
        pop(vm);
        break;
      }

      case OP_STRINGIFY: {
        if (!IS_STRING(peek(vm, 0)) && !IS_NIL(peek(vm, 0))) {
          b_obj_string *value = value_to_string(vm, pop(vm));
          if (value->length != 0) {
            push(vm, OBJ_VAL(value));
          } else {
            push(vm, NIL_VAL);
          }
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
        if(IS_EMPTY(peek(vm, 0))) {
          runtime_error(ERR_CANT_ASSIGN_EMPTY);
          break;
        }
        table_set(vm, &vm->current_frame->closure->function->module->values, OBJ_VAL(name), peek(vm, 0));
        pop(vm);

#if defined(DEBUG_TABLE) && DEBUG_TABLE
        table_print(&vm->globals);
#endif
        break;
      }

      case OP_GET_GLOBAL: {
        b_obj_string *name = READ_STRING();
        b_value value;
        if (!table_get(&vm->current_frame->closure->function->module->values, OBJ_VAL(name), &value)) {
          if (!table_get(&vm->globals, OBJ_VAL(name), &value)) {
            dbg(printf("Name requested: '%s' with length %d\n", name->chars, name->length));
            cond_dbg(vm->current_frame, table_print(&vm->current_frame->closure->function->module->values));

            runtime_error("'%s' is undefined in this scope", name->chars);
            break;
          }
        }
        push(vm, value);
        break;
      }

      case OP_SET_GLOBAL: {
        if(IS_EMPTY(peek(vm, 0))) {
          runtime_error(ERR_CANT_ASSIGN_EMPTY);
          break;
        }

        b_obj_string *name = READ_STRING();
        b_table *table = &vm->current_frame->closure->function->module->values;
        if (table_set(vm, table, OBJ_VAL(name), peek(vm, 0))) {
          table_delete(table, OBJ_VAL(name));
          runtime_error("%s is undefined in this scope", name->chars);
          break;
        }
        break;
      }

      case OP_GET_LOCAL: {
        uint16_t slot = READ_SHORT();
        push(vm, vm->current_frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint16_t slot = READ_SHORT();
        if(IS_EMPTY(peek(vm, 0))) {
          runtime_error(ERR_CANT_ASSIGN_EMPTY);
          break;
        }
        vm->current_frame->slots[slot] = peek(vm, 0);
        break;
      }

      case OP_GET_PROPERTY: {
        b_obj_string *name = READ_STRING();

        if (IS_OBJ(peek(vm, 0))) {
          b_value value;

          switch (AS_OBJ(peek(vm, 0))->type) {
            case OBJ_MODULE: {
              b_obj_module *module = AS_MODULE(peek(vm, 0));
              if (table_get(&module->values, OBJ_VAL(name), &value)) {
                if (is_private(name)) {
                  runtime_error("cannot get private module property '%s'", name->chars);
                  break;
                }

                pop(vm); // pop the module...
                push(vm, value);
                break;
              }

              runtime_error("%s module does not define '%s'", module->name, name->chars);
              break;
            }
            case OBJ_CLASS: {
              if (table_get(&AS_CLASS(peek(vm, 0))->methods, OBJ_VAL(name), &value)) {
                if (get_method_type(value) == TYPE_STATIC) {
                  if (is_private(name)) {
                    runtime_error("cannot call private property '%s' of class %s",
                                  name->chars, AS_CLASS(peek(vm, 0))->name->chars);
                    break;
                  }
                  pop(vm); // pop the class...
                  push(vm, value);
                  break;
                }
              } else if (table_get(&AS_CLASS(peek(vm, 0))->static_properties, OBJ_VAL(name), &value)) {
                if (is_private(name)) {
                  runtime_error("cannot call private property '%s' of class %s",
                                name->chars, AS_CLASS(peek(vm, 0))->name->chars);
                  break;
                }
                pop(vm); // pop the class...
                push(vm, value);
                break;
              }

              runtime_error("class %s does not have a static property or method named '%s'",
                            AS_CLASS(peek(vm, 0))->name->chars, name->chars);
              break;
            }
            case OBJ_INSTANCE: {
              b_obj_instance *instance = AS_INSTANCE(peek(vm, 0));
              if (table_get(&instance->properties, OBJ_VAL(name), &value)) {
                if (is_private(name)) {
                  runtime_error("cannot call private property '%s' from instance of %s",
                                name->chars, instance->klass->name->chars);
                  break;
                }
                pop(vm); // pop the instance...
                push(vm, value);
                break;
              }

              if (is_private(name)) {
                runtime_error("cannot bind private property '%s' to instance of %s",
                              name->chars, instance->klass->name->chars);
                break;
              }

              if (bind_method(vm, instance->klass, name)) {
                break;
              }

              runtime_error("instance of class %s does not have a property or method named '%s'",
                            AS_INSTANCE(peek(vm, 0))->klass->name->chars, name->chars);
              break;
            }
            case OBJ_STRING: {
              if (table_get(&vm->methods_string, OBJ_VAL(name), &value)) {
                pop(vm); // pop the string...
                push(vm, value);
                break;
              }

              runtime_error("class String has no named property '%s'", name->chars);
              break;
            }
            case OBJ_LIST: {
              if (table_get(&vm->methods_list, OBJ_VAL(name), &value)) {
                pop(vm); // pop the list...
                push(vm, value);
                break;
              }

              runtime_error("class List has no named property '%s'", name->chars);
              break;
            }
            case OBJ_RANGE: {
              if (table_get(&vm->methods_range, OBJ_VAL(name), &value)) {
                pop(vm); // pop the range...
                push(vm, value);
                break;
              }

              runtime_error("class Range has no named property '%s'", name->chars);
              break;
            }
            case OBJ_DICT: {
              if (table_get(&AS_DICT(peek(vm, 0))->items, OBJ_VAL(name), &value) ||
                  table_get(&vm->methods_dict, OBJ_VAL(name), &value)) {
                pop(vm); // pop the dictionary...
                push(vm, value);
                break;
              }

              runtime_error("unknown key or class Dict property '%s'", name->chars);
              break;
            }
            case OBJ_BYTES: {
              if (table_get(&vm->methods_bytes, OBJ_VAL(name), &value)) {
                pop(vm); // pop the bytes...
                push(vm, value);
                break;
              }

              runtime_error("class Bytes has no named property '%s'", name->chars);
              break;
            }
            case OBJ_FILE: {
              if (table_get(&vm->methods_file, OBJ_VAL(name), &value)) {
                pop(vm); // pop the file...
                push(vm, value);
                break;
              }

              runtime_error("class File has no named property '%s'", name->chars);
              break;
            }
            default: {
              runtime_error("object of type %s does not carry properties", value_type(peek(vm, 0)));
              break;
            }
          }
        } else {
          runtime_error("'%s' of type %s does not have properties", value_to_string(vm, peek(vm, 0))->chars, value_type(peek(vm, 0)));
          break;
        }
        break;
      }

      case OP_GET_SELF_PROPERTY: {
        b_obj_string *name = READ_STRING();
        b_value value;

        if (IS_INSTANCE(peek(vm, 0))) {
          b_obj_instance *instance = AS_INSTANCE(peek(vm, 0));
          if (table_get(&instance->properties, OBJ_VAL(name), &value)) {
            pop(vm); // pop the instance...
            push(vm, value);
            break;
          }

          if (bind_method(vm, instance->klass, name)) {
            break;
          }

          runtime_error("instance of class %s does not have a property or method named '%s'",
                        AS_INSTANCE(peek(vm, 0))->klass->name->chars, name->chars);
          break;
        } else if (IS_CLASS(peek(vm, 0))) {
          b_obj_class *klass = AS_CLASS(peek(vm, 0));
          if (table_get(&klass->methods, OBJ_VAL(name), &value)) {
            if (get_method_type(value) == TYPE_STATIC) {
              pop(vm); // pop the class...
              push(vm, value);
              break;
            }
          } else if (table_get(&klass->static_properties, OBJ_VAL(name), &value)) {
            pop(vm); // pop the class...
            push(vm, value);
            break;
          }
          runtime_error("class %s does not have a static property or method named '%s'",
                        klass->name->chars, name->chars);
          break;
        } else if (IS_MODULE(peek(vm, 0))) {
          b_obj_module *module = AS_MODULE(peek(vm, 0));
          if (table_get(&module->values, OBJ_VAL(name), &value)) {
            pop(vm); // pop the module...
            push(vm, value);
            break;
          }

          runtime_error("module %s does not define '%s'", module->name, name->chars);
          break;
        }

        runtime_error("'%s' of type %s does not have properties", value_to_string(vm, peek(vm, 0))->chars, value_type(peek(vm, 0)));
        break;
      }

      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(vm, 1)) && !IS_DICT(peek(vm, 1))) {
          runtime_error("object of type %s can not carry properties", value_type(peek(vm, 1)));
          break;
        } else  if(IS_EMPTY(peek(vm, 0))) {
          runtime_error(ERR_CANT_ASSIGN_EMPTY);
          break;
        }

        b_obj_string *name = READ_STRING();

        if (IS_INSTANCE(peek(vm, 1))) {
          b_obj_instance *instance = AS_INSTANCE(peek(vm, 1));
          table_set(vm, &instance->properties, OBJ_VAL(name), peek(vm, 0));

          b_value value = pop(vm);
          pop(vm); // removing the instance object
          push(vm, value);
        } else {
          b_obj_dict *dict = AS_DICT(peek(vm, 1));
          dict_set_entry(vm, dict, OBJ_VAL(name), peek(vm, 0));

          b_value value = pop(vm);
          pop(vm); // removing the dictionary object
          push(vm, value);
        }
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
            closure->up_values[i] = capture_up_value(vm, vm->current_frame->slots + index);
          } else {
            closure->up_values[i] =
                ((b_obj_closure *) vm->current_frame->closure)->up_values[index];
          }
        }

        break;
      }
      case OP_GET_UP_VALUE: {
        int index = READ_SHORT();
        push(vm, *((b_obj_closure *) vm->current_frame->closure)->up_values[index]->location);
        break;
      }
      case OP_SET_UP_VALUE: {
        int index = READ_SHORT();
        if(IS_EMPTY(peek(vm, 0))) {
          runtime_error(ERR_CANT_ASSIGN_EMPTY);
          break;
        }
        *((b_obj_closure *) vm->current_frame->closure)->up_values[index]->location =
            peek(vm, 0);
        break;
      }

      case OP_CALL: {
        int arg_count = READ_BYTE();
        if (!call_value(vm, peek(vm, arg_count), arg_count)) {
          EXIT_VM();
        }
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }
      case OP_INVOKE: {
        b_obj_string *method = READ_STRING();
        int arg_count = READ_BYTE();
        if (!invoke(vm, method, arg_count)) {
          EXIT_VM();
        }
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }
      case OP_INVOKE_SELF: {
        b_obj_string *method = READ_STRING();
        int arg_count = READ_BYTE();
        if (!invoke_self(vm, method, arg_count)) {
          EXIT_VM();
        }
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }

      case OP_CLASS: {
        b_obj_string *name = READ_STRING();
        push(vm, OBJ_VAL(new_class(vm, name)));
        break;
      }
      case OP_METHOD: {
        b_obj_string *name = READ_STRING();
        define_method(vm, name);
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
          break;
        }

        b_obj_class *superclass = AS_CLASS(peek(vm, 1));
        b_obj_class *subclass = AS_CLASS(peek(vm, 0));
        table_add_all(vm, &superclass->properties, &subclass->properties);
        table_add_all(vm, &superclass->methods, &subclass->methods);
        subclass->superclass = superclass;
        pop(vm); // pop the subclass
        break;
      }
      case OP_GET_SUPER: {
        b_obj_string *name = READ_STRING();
        b_obj_class *klass = AS_CLASS(peek(vm, 0));
        if (!bind_method(vm, klass->superclass, name)) {
          runtime_error("class %s does not define a function %s", klass->name->chars, name->chars);
        }
        break;
      }
      case OP_SUPER_INVOKE: {
        b_obj_string *method = READ_STRING();
        int arg_count = READ_BYTE();
        b_obj_class *klass = AS_CLASS(pop(vm));
        if (!invoke_from_class(vm, klass, method, arg_count)) {
          EXIT_VM();
        }
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }
      case OP_SUPER_INVOKE_SELF: {
        int arg_count = READ_BYTE();
        b_obj_class *klass = AS_CLASS(pop(vm));
        if (!invoke_from_class(vm, klass, klass->name, arg_count)) {
          EXIT_VM();
        }
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }

      case OP_LIST: {
        int count = READ_SHORT();
        b_obj_list *list = new_list(vm);
        vm->stack_top[-count - 1] = OBJ_VAL(list);

        for (int i = count - 1; i >= 0; i--) {
          write_list(vm, list, peek(vm, i));
        }
        pop_n(vm, count);
        break;
      }
      case OP_RANGE: {
        b_value _upper = peek(vm, 0), _lower = peek(vm, 1);

        if (!IS_NUMBER(_upper) || !IS_NUMBER(_lower)) {
          runtime_error("invalid range boundaries");
          break;
        }

        double lower = AS_NUMBER(_lower), upper = AS_NUMBER(_upper);
        pop_n(vm, 2);
        push(vm, OBJ_VAL(new_range(vm, lower, upper)));
        break;
      }
      case OP_DICT: {
        int count = READ_SHORT() * 2; // 1 for key, 1 for value
        b_obj_dict *dict = new_dict(vm);
        vm->stack_top[-count - 1] = OBJ_VAL(dict);

        for (int i = 0; i < count; i += 2) {
          b_value name = vm->stack_top[-count + i];
          if(!IS_STRING(name) && !IS_NUMBER(name) && !IS_BOOL(name)) {
            runtime_error("dictionary key must be one of string, number or boolean");
          }
          b_value value = vm->stack_top[-count + i + 1];
          dict_set_entry(vm, dict, name, value);
        }
        pop_n(vm, count);
        break;
      }

      case OP_GET_RANGED_INDEX: {
        uint8_t will_assign = READ_BYTE();

        bool is_gotten = true;
        if (IS_OBJ(peek(vm, 2))) {
          switch (AS_OBJ(peek(vm, 2))->type) {
            case OBJ_STRING: {
              if (!string_get_ranged_index(vm, AS_STRING(peek(vm, 2)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_LIST: {
              if (!list_get_ranged_index(vm, AS_LIST(peek(vm, 2)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_BYTES: {
              if (!bytes_get_ranged_index(vm, AS_BYTES(peek(vm, 2)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            default: {
              is_gotten = false;
              break;
            }
          }
        } else {
          is_gotten = false;
        }

        if (!is_gotten) {
          runtime_error("cannot range index object of type %s", value_type(peek(vm, 2)));
        }
        break;
      }
      case OP_GET_INDEX: {
        uint8_t will_assign = READ_BYTE();

        bool is_gotten = true;
        if (IS_OBJ(peek(vm, 1))) {
          switch (AS_OBJ(peek(vm, 1))->type) {
            case OBJ_STRING: {
              if (!string_get_index(vm, AS_STRING(peek(vm, 1)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_LIST: {
              if (!list_get_index(vm, AS_LIST(peek(vm, 1)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_DICT: {
              if (!dict_get_index(vm, AS_DICT(peek(vm, 1)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_MODULE: {
              if (!module_get_index(vm, AS_MODULE(peek(vm, 1)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_BYTES: {
              if (!bytes_get_index(vm, AS_BYTES(peek(vm, 1)), will_assign == (uint8_t) 1)) {
                EXIT_VM();
              }
              break;
            }
            default: {
              is_gotten = false;
              break;
            }
          }
        } else {
          is_gotten = false;
        }

        if (!is_gotten) {
          runtime_error("cannot index object of type %s", value_type(peek(vm, 1)));
        }
        break;
      }

      case OP_SET_INDEX: {
        bool is_set = true;
        if (IS_OBJ(peek(vm, 2))) {

          b_value value = peek(vm, 0);
          b_value index = peek(vm, 1);

          if(IS_EMPTY(value)) {
            runtime_error(ERR_CANT_ASSIGN_EMPTY);
            break;
          }

          switch (AS_OBJ(peek(vm, 2))->type) {
            case OBJ_LIST: {
              if (!list_set_index(vm, AS_LIST(peek(vm, 2)), index, value)) {
                EXIT_VM();
              }
              break;
            }
            case OBJ_STRING: {
              runtime_error("strings do not support object assignment");
              break;
            }
            case OBJ_DICT: {
              dict_set_index(vm, AS_DICT(peek(vm, 2)), index, value);
              break;
            }
            case OBJ_MODULE: {
              module_set_index(vm, AS_MODULE(peek(vm, 2)), index, value);
              break;
            }
            case OBJ_BYTES: {
              if (!bytes_set_index(vm, AS_BYTES(peek(vm, 2)), index, value)) {
                EXIT_VM();
              }
              break;
            }
            default: {
              is_set = false;
              break;
            }
          }
        } else {
          is_set = false;
        }

        if (!is_set) {
          runtime_error("type of %s is not a valid iterable", value_type(peek(vm, 3)));
        }
        break;
      }

      case OP_RETURN: {
        b_value result = pop(vm);

        close_up_values(vm, vm->current_frame->slots);

        vm->frame_count--;
        if (vm->frame_count == 0) {
          pop(vm);
          return PTR_OK;
        }

        vm->stack_top = vm->current_frame->slots;
        push(vm, result);

        vm->current_frame = &vm->frames[vm->frame_count - 1];

        if (vm->frame_count == exit_frame) {
          return PTR_OK;
        }

        break;
      }

      case OP_CALL_IMPORT: {
        b_obj_closure *closure = AS_CLOSURE(READ_CONSTANT());
        add_module(vm, closure->function->module);
        register_module__FILE__(vm, closure->function->module);
        call(vm, closure, 0);
        vm->current_frame = &vm->frames[vm->frame_count - 1];
        break;
      }

      case OP_NATIVE_MODULE: {
        b_obj_string *module_name = READ_STRING();
        b_value value;
        if (table_get(&vm->modules, OBJ_VAL(module_name), &value)) {
          b_obj_module *module = AS_MODULE(value);
          if(module->preloader != NULL) {
            ((b_module_loader)module->preloader)(vm);
          }
          module->imported = true;
          table_set(vm, &vm->current_frame->closure->function->module->values, OBJ_VAL(module_name), value);
          break;
        }
        runtime_error("module '%s' not found", module_name->chars);
        break;
      }

      case OP_SELECT_IMPORT: {
        b_obj_string *entry_name = READ_STRING();
        b_obj_func *function = AS_CLOSURE(peek(vm, 0))->function;
        b_value value;
        if (table_get(&function->module->values, OBJ_VAL(entry_name), &value)) {
          table_set(vm, &vm->current_frame->closure->function->module->values, OBJ_VAL(entry_name), value);
        } else {
          runtime_error("module %s does not define '%s'", function->module->name, entry_name->chars);
        }
        break;
      }

      case OP_SELECT_NATIVE_IMPORT: {
        b_obj_string *module_name = AS_STRING(peek(vm, 0));
        b_obj_string *value_name = READ_STRING();
        b_value mod;
        if (table_get(&vm->modules, OBJ_VAL(module_name), &mod)) {
          b_obj_module *module = AS_MODULE(mod);
          b_value value;
          if (table_get(&module->values, OBJ_VAL(value_name), &value)) {
            table_set(vm, &vm->current_frame->closure->function->module->values, OBJ_VAL(value_name), value);
          } else {
            runtime_error("module %s does not define '%s'", module->name, value_name->chars);
          }
        } else{
          runtime_error("module '%s' not found", module_name->chars);
        }
        break;
      }

      case OP_IMPORT_ALL: {
        table_import_all(vm, &AS_CLOSURE(peek(vm, 0))->function->module->values, &vm->current_frame->closure->function->module->values);
        break;
      }

      case OP_IMPORT_ALL_NATIVE: {
        b_obj_string *name = AS_STRING(peek(vm, 0));
        b_value mod;
        if (table_get(&vm->modules, OBJ_VAL(name), &mod)) {
          table_import_all(vm, &AS_MODULE(mod)->values, &vm->current_frame->closure->function->module->values);
        }
        break;
      }

      case OP_EJECT_IMPORT: {
        b_obj_func *function = AS_CLOSURE(READ_CONSTANT())->function;
        b_table *current_module = &vm->current_frame->closure->function->module->values;
        b_value module_name = STRING_VAL(function->module->name);

        b_value tmp;
        if(table_get(current_module, module_name, &tmp)) {
          if(!IS_MODULE(tmp)) {
            break;
          }
        }

        table_delete(current_module, module_name);
        break;
      }

      case OP_EJECT_NATIVE_IMPORT: {
        b_value mod;
        b_obj_string *name = READ_STRING();
        if (table_get(&vm->modules, OBJ_VAL(name), &mod)) {
          b_table *current_module = &vm->current_frame->closure->function->module->values;

          table_import_all(vm, &AS_MODULE(mod)->values, current_module);
          table_delete(current_module, OBJ_VAL(name));
        }
        break;
      }

      case OP_ASSERT: {
        b_value message = pop(vm);
        b_value expression = pop(vm);
        if (is_false(expression)) {
          if (!IS_NIL(message)) {
            do_throw_exception(vm, true, value_to_string(vm, message)->chars);
          } else {
            do_throw_exception(vm, true, "");
          }
        }
        break;
      }

      case OP_DIE: {
        if (!IS_INSTANCE(peek(vm, 0)) ||
            !is_instance_of(AS_INSTANCE(peek(vm, 0))->klass, vm->exception_class->name->chars)) {
          runtime_error("instance of Exception expected");
          break;
        }

        b_value exception = peek(vm, 0);
        b_value stacktrace = get_stack_trace(vm);
        b_obj_instance *instance = AS_INSTANCE(exception);
        table_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
        table_set(vm, &instance->properties, STRING_L_VAL("type", 4),
          STRING_L_VAL(instance->klass->name->chars, instance->klass->name->length)
        );
        pop(vm); // pop the exception

        if(print_exception(vm, instance, false)) {
          b_error_frame *error = peek_error(vm);
          vm->frame_count -= vm->current_frame - error->frame;
          vm->current_frame = error->frame;
          vm->current_frame->ip = &error->frame->closure->function->blob.code[error->offset];
          break;
        }

        EXIT_VM();
        break;
      }

      case OP_SWITCH: {
        b_obj_switch *sw = AS_SWITCH(READ_CONSTANT());
        b_value expr = peek(vm, 0);

        b_value value;
        if (table_get(&sw->table, expr, &value)) {
          vm->current_frame->ip += (int) AS_NUMBER(value);
        } else if (sw->default_jump != -1) {
          vm->current_frame->ip += sw->default_jump;
        } else {
          vm->current_frame->ip += sw->exit_jump;
        }
        pop(vm);
        break;
      }

      case OP_CHOICE: {
        b_value _else = peek(vm, 0);
        b_value _then = peek(vm, 1);
        b_value _condition = peek(vm, 2);

        pop_n(vm, 3);
        if (!is_false(_condition)) {
          push(vm, _then);
        } else {
          push(vm, _else);
        }
        break;
      }

      case OP_BEGIN_CATCH: {
        uint16_t offset = READ_SHORT();

        b_error_frame *error = ALLOCATE(b_error_frame, 1);
        error->frame = vm->current_frame;
        error->offset = offset;
        error->value = NIL_VAL;

        push_error(vm, error);
        break;
      }

      case OP_END_CATCH: {
        b_error_frame *error = pop_error(vm);
        push(vm, error->value);
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

void register_module__FILE__(b_vm *vm, b_obj_module *module) {
  // register module __file__
  push(vm, STRING_L_VAL("__file__", 8));
  push(vm, STRING_VAL(module->file));
  table_set(vm, &module->values, peek(vm, 1), peek(vm, 0));
  pop_n(vm, 2);
}

// helper function to access call outside the vm file.
b_value raw_closure_call(b_vm *vm, b_obj_closure *closure, b_obj_list *args, bool must_push) {
  b_value *stack_top = vm->stack_top;

  // set the closure before the args
  if(must_push) {
    push(vm, OBJ_VAL(closure));
  }

  int arg_count = 0;
  if(args && (arg_count = args->items.count)) {
    for(int i = 0; i < args->items.count; i++) {
      push(vm, args->items.values[i]);
    }
  }

  call(vm, closure, arg_count);
  b_ptr_result vm_result = run(vm, vm->frame_count - 1);

  if(vm_result != PTR_OK) {
    exit(EXIT_RUNTIME);
  }

  b_value result = vm->stack_top[-1];
  pop_n(vm, arg_count + 1);

  vm->stack_top = stack_top;
  return result;
}

b_value call_closure(b_vm *vm, b_obj_closure *closure, b_obj_list *args) {
  return raw_closure_call(vm, closure, args, true);
}

// helper function to access call outside the vm file.
bool queue_closure(b_vm *vm, b_obj_closure *closure) {
  return call(vm, closure, 0);
}

b_ptr_result interpret(b_vm *vm, b_obj_module *module, const char *source) {
  push(vm, OBJ_VAL(module));

  if(vm->exception_class == NULL) {
    initialize_exceptions(vm, module);
  }

  b_obj_func *function = compile(vm, module, source);
  if(function == NULL) {
    return PTR_COMPILE_ERR;
  }

  if (vm->should_exit_after_bytecode) {
    return PTR_OK;
  }

  push(vm, OBJ_VAL(function));
  b_obj_closure *closure = new_closure(vm, function);
  pop(vm);
  push(vm, OBJ_VAL(closure));

  register_module__FILE__(vm, module);

  call(vm, closure, 0);
  return run(vm, 0);
}

#undef ERR_CANT_ASSIGN_EMPTY
