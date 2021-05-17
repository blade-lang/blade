#ifndef bird_vm_h
#define bird_vm_h

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
  b_obj_class *runtime_exception_class;

  // gc
  int gray_count;
  int gray_capacity;
  b_obj **gray_stack;
  size_t bytes_allocated;
  size_t next_gc;

  // object methods
  b_table methods_string;
  b_table methods_list;
  b_table methods_dict;
  b_table methods_file;
  b_table methods_bytes;

  // repl flag
  bool is_repl;
};

void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
b_ptr_result interpret(b_vm *vm, const char *source, const char *filename);
void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);
b_value pop_n(b_vm *vm, int n);
b_value peek(b_vm *vm, int distance);

bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name,
                       int arg_count);
bool is_falsey(b_value value);
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

void gc_start_protect(b_vm *vm);
void gc_stop_protection(b_vm *vm);

#define GUARD(t, n, v) t *n = v; \
  push(vm, OBJ_VAL(n))

#endif