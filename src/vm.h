#ifndef BLADE_VM_H
#define BLADE_VM_H

typedef struct s_compiler b_compiler;

#include "blob.h"
#include "config.h"
#include "object.h"
#include "table.h"
#include "value.h"

typedef enum {
  PTR_OK,
  PTR_COMPILE_ERR,
  PTR_RUNTIME_ERR,
} b_ptr_result;

typedef struct {
  b_obj_closure *closure;
  uint8_t *ip;
  b_value *slots;
  int gc_protected;
} b_call_frame;

typedef struct {
  b_call_frame *frame;
  uint16_t offset;
  b_value *stack_head;
  b_value value;
} b_error_frame;

struct s_vm {
  b_call_frame frames[FRAMES_MAX];
  b_call_frame *current_frame;
  int frame_count;

  b_blob *blob;
  uint8_t *ip;
  b_obj_up_value *open_up_values;

  b_error_frame *errors[ERRORS_MAX];
  int error_count;

  size_t stack_capacity;
  b_value *stack;
  b_value *stack_top;

  b_obj *objects;
  b_compiler *compiler;
  b_obj_class *exception_class;
  char *root_file;

  // gc
  int gray_count;
  int gray_capacity;
  b_obj **gray_stack;
  size_t bytes_allocated;
  size_t next_gc;

  // objects tracker
  b_table modules;
  b_table strings;
  b_table globals;

  // object public methods
  b_table methods_string;
  b_table methods_list;
  b_table methods_dict;
  b_table methods_file;
  b_table methods_bytes;
  b_table methods_range;

  char **std_args;
  int std_args_count;

  // boolean flags
  bool is_repl;
  bool mark_value;
  // for switching through the command line args...
  bool show_warnings;
  bool should_print_bytecode;
  bool should_exit_after_bytecode;

  // id
  uint64_t id;
  b_vm *parent_vm;
};

void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
void register__ROOT__(b_vm *vm);

b_ptr_result interpret(b_vm *vm, b_obj_module *module, const char *source);

void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);
b_value pop_n(b_vm *vm, int n);
b_value peek(b_vm *vm, int distance);

void push_error(b_vm *vm, b_error_frame *frame);
b_error_frame* pop_error(b_vm *vm);
b_error_frame* peek_error(b_vm *vm);

static inline void add_module(b_vm *vm, b_obj_module *module) {
  cond_dbg(vm->current_frame, printf("Adding module %s from %s to %s in %s\n", 
    module->name, 
    module->file, 
    vm->current_frame->closure->function->module->name, 
    vm->current_frame->closure->function->module->file
  ));

  
  table_set(vm, &vm->modules, STRING_VAL(module->file), OBJ_VAL(module));
  if (vm->frame_count == 0) {
    table_set(vm, &vm->globals, STRING_VAL(module->name), OBJ_VAL(module));
  } else {
    table_set(vm, &vm->current_frame->closure->function->module->values,
              STRING_VAL(module->name), OBJ_VAL(module));
  }
}

static inline void add_known_module(b_vm *vm, b_obj_module *module, char *name) {
  cond_dbg(vm->current_frame, printf("Adding known module %s from %s to %s in %s as %s\n", 
    module->name, 
    module->file, 
    vm->current_frame->closure->function->module->name, 
    vm->current_frame->closure->function->module->file,
    name
  ));

  if (vm->frame_count == 0) {
    table_set(vm, &vm->globals, STRING_VAL(name), OBJ_VAL(module));
  } else {
    table_set(vm, &vm->current_frame->closure->function->module->values,
              STRING_VAL(name), OBJ_VAL(module));
  }
}

bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name, int arg_count);

void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);
bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value);
bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);
void define_native_method(b_vm *vm, b_table *table, const char *name,
                          b_native_fn function);

bool is_false(b_value value);
bool is_instance_of(b_obj_class *klass1, b_obj_class *klass2);

bool do_throw_exception(b_vm *vm, const char *type, bool is_assert, const char *format, ...);
b_obj_instance *create_exception(b_vm *vm, const char* type, b_obj_string *message);

#define EXIT_VM() return PTR_RUNTIME_ERR

#define base_runtime_error(s, ...)  do {                                               \
  if(!do_throw_exception(vm, (s), false, ##__VA_ARGS__)){                \
    EXIT_VM(); \
  }} while(0)

#define runtime_error(...)  base_runtime_error(NULL, ##__VA_ARGS__)
#define numeric_error(...)  base_runtime_error("NumericError", ##__VA_ARGS__)
#define argument_error(...)  base_runtime_error("ArgumentError", ##__VA_ARGS__)
#define value_error(...)  base_runtime_error("ValueError", ##__VA_ARGS__)
#define type_error(...)  base_runtime_error("TypeError", ##__VA_ARGS__)
#define range_error(...)  base_runtime_error("RangeError", ##__VA_ARGS__)
#define access_error(...)  base_runtime_error("AccessError", ##__VA_ARGS__)
#define property_error(...)  base_runtime_error("PropertyError", ##__VA_ARGS__)
#define undefined_error(...)  base_runtime_error("UndefinedError", ##__VA_ARGS__)

#define throw_exception(v, ...) do_throw_exception(v, NULL, false, ##__VA_ARGS__)
#define throw_numeric_error(v, ...) do_throw_exception(v, "NumericError", false, ##__VA_ARGS__)
#define throw_argument_error(v, ...) do_throw_exception(v, "ArgumentError", false, ##__VA_ARGS__)
#define throw_value_error(v, ...) do_throw_exception(v, "ValueError", false, ##__VA_ARGS__)
#define throw_type_error(v, ...) do_throw_exception(v, "TypeError", false, ##__VA_ARGS__)
#define throw_range_error(v, ...) do_throw_exception(v, "RangeError", false, ##__VA_ARGS__)
#define throw_access_error(v, ...) do_throw_exception(v, "AccessError", false, ##__VA_ARGS__)
#define throw_property_error(v, ...) do_throw_exception(v, "PropertyError", false, ##__VA_ARGS__)
#define throw_undefined_error(v, ...) do_throw_exception(v, "UndefinedError", false, ##__VA_ARGS__)

static inline b_obj *gc_protect(b_vm *vm, b_obj *object) {
  push(vm, OBJ_VAL(object));
  vm->frames[vm->frame_count > 0 ? vm->frame_count - 1 : 0].gc_protected++;
  return object;
}

static inline void gc_clear_protection(b_vm *vm) {
  b_call_frame *frame = &vm->frames[vm->frame_count > 0 ? vm->frame_count - 1 : 0];
  if (frame->gc_protected > 0) {
    pop_n(vm, frame->gc_protected);
  }
  frame->gc_protected = 0;
}

// NOTE:
// 1. Any call to GC() within a function/block must be accompanied by
// at least one call to CLEAR_GC() before exiting the function/block
// otherwise, expected unexpected behavior
// 2. The call to CLEAR_GC() will be automatic for native functions.
// 3. METHOD_OBJECT must be retrieved before any call to GC() in a
// native function.
#define GC(o) gc_protect(vm, (b_obj*)(o))
#define CLEAR_GC() gc_clear_protection(vm)

bool call_value(b_vm *vm, b_value callee, int arg_count);
b_value raw_closure_call(b_vm *vm, b_obj_closure *closure, b_obj_list *args, bool must_push);
b_value call_closure(b_vm *vm, b_obj_closure *closure, b_obj_list *args);
bool queue_closure(b_vm *vm, b_obj_closure *closure);
b_ptr_result run_closure_call(b_vm *vm, b_obj_closure *closure, b_obj_list *args);
void register_module__FILE__(b_vm *vm, b_obj_module *module);

#endif