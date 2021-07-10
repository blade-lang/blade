#ifndef BIRD_VM_H
#define BIRD_VM_H

typedef struct s_compiler b_compiler;

#include "blob.h"
#include "compiler.h"
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
  b_obj *function;
  uint8_t *ip;
  b_value *slots;
  int handlers_count;
  b_exception_frame handlers[MAX_EXCEPTION_HANDLERS];
} b_call_frame;

struct s_vm {
  b_call_frame frames[FRAMES_MAX];
  int frame_count;

  b_blob *blob;
  uint8_t *ip;
  b_value stack[STACK_MAX];
  b_value *stack_top;
  b_table strings;
  b_table bytes;
  b_table globals;
  b_obj_up_value *open_up_values;

  b_obj *objects;
  b_compiler *compiler;
  b_obj_class *exception_class;

  // gc
  int gray_count;
  int gray_capacity;
  int gc_protected;
  b_obj **gray_stack;
  size_t bytes_allocated;
  size_t next_gc;

  // object public methods
  b_table methods_string;
  b_table methods_list;
  b_table methods_dict;
  b_table methods_file;
  b_table methods_bytes;

  // boolean flags
  bool is_repl;
  bool mark_value;
  // for switching through the command line args...
  bool should_debug_stack;
  bool should_print_bytecode;
};

void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
b_ptr_result interpret(b_vm *vm, const char *source, const char *filename);
void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);
b_value pop_n(b_vm *vm, int n);
b_value peek(b_vm *vm, int distance);

bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name, int arg_count);
bool is_false(b_value value);
void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);
bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value);
bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);

void define_native_method(b_vm *vm, b_table *table, const char *name,
                          b_native_fn function);
bool is_instance_of(b_obj_class *klass1, char *klass2_name);

bool throw_exception(b_vm *vm, const char *format, ...); 
void _runtime_error(b_vm *vm, const char *format, ...);
b_obj_instance *create_exception(b_vm *vm, b_obj_string *message);

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...)                                                     \
  if(!throw_exception(vm, ##__VA_ARGS__)){                                     \
    EXIT_VM(); \
  }


static inline b_obj *gc_protect(b_vm *vm, b_obj *object) {
  push(vm, OBJ_VAL(object));
  vm->gc_protected++; 
  return object;
}

static inline void gc_clear_protection(b_vm *vm) {
  if(vm->gc_protected > 0) {
    vm->stack_top -= vm->gc_protected;
  }
  vm->gc_protected = 0;
}

// NOTE:
// any call to GC() within a function/block must accompanied by
// at least one call to CLEAR_GC() before exiting the function/block
// otherwise, expected unexpected behavior
// NOTE as well that the call to CLEAR_GC() will be automatic for
// native functions.
// NOTE as well that METHOD_OBJECT must be retrieved before any call
// to GC() in a native function.
#define GC(o) gc_protect(vm, (b_obj*)(o))
#define CLEAR_GC() gc_clear_protection(vm)

#endif