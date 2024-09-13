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
  uint16_t address;
  uint16_t finally_address;
  b_obj_class *klass;
} b_exception_frame;

typedef struct {
  b_obj_closure *closure;
  uint8_t *ip;
  b_value *slots;
  int handlers_count;
  int gc_protected;
  b_exception_frame handlers[MAX_EXCEPTION_HANDLERS];
} b_call_frame;

struct s_vm {
  b_call_frame frames[FRAMES_MAX];
  b_call_frame *current_frame;
  unsigned int frame_count;

  b_blob *blob;
  uint8_t *ip;
  b_obj_up_value *open_up_values;

  size_t stack_capacity;
  size_t stack_count;
  b_value *stack;
  b_value *stack_top;


  b_obj *objects;
  b_compiler *compiler;
  b_obj_class *exception_class;
  char *root_file;

  // gc
  int gray_count;
  int gray_capacity;
  int gc_protected;
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

  // miscellaneous
};

void init_vm(b_vm *vm);

void free_vm(b_vm *vm);

b_ptr_result interpret(b_vm *vm, b_obj_module *module, const char *source);

void push(b_vm *vm, b_value value);

b_value pop(b_vm *vm);

b_value pop_n(b_vm *vm, int n);

b_value peek(b_vm *vm, int distance);

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

bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name, int arg_count);

bool is_false(b_value value);

void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);

bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value);

bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);

void define_native_method(b_vm *vm, b_table *table, const char *name,
                          b_native_fn function);

bool is_instance_of(b_obj_class *klass1, char *klass2_name);

bool do_throw_exception(b_vm *vm, bool is_assert, const char *format, ...);

void do_runtime_error(b_vm *vm, const char *format, ...);

b_obj_instance *create_exception(b_vm *vm, b_obj_string *message);

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...)                                                     \
  if(!throw_exception(vm, ##__VA_ARGS__)){                                     \
    EXIT_VM(); \
  }

#define throw_exception(v, ...) do_throw_exception(v, false, ##__VA_ARGS__)

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

b_value call_closure(b_vm *vm, b_obj_closure *closure, b_obj_list *args);
bool queue_closure(b_vm *vm, b_obj_closure *closure);
void register_module__FILE__(b_vm *vm, b_obj_module *module);

#endif