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
  b_obj_closure *closure;
  uint8_t *ip;
  b_value *slots;
} b_call_frame;

struct s_vm {
  b_call_frame frames[FRAMES_MAX];
  int frame_count;

  b_blob *blob;
  uint8_t *ip;
  b_value stack[STACK_MAX];
  b_value *stack_top;
  b_table strings;
  b_table globals;
  b_obj_upvalue *open_upvalues;

  b_obj *objects;
  b_compiler *compiler;

  // gc
  int gray_count;
  int gray_capacity;
  b_obj **gray_stack;
  size_t bytes_allocated;
  size_t next_gc;
};

void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
b_ptr_result interpret(b_vm *vm, const char *source);
void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);

void _runtime_error(b_vm *vm, const char *format, ...);

#define runtime_error(...)                                                     \
  _runtime_error(vm, ##__VA_ARGS__);                                           \
  return PTR_RUNTIME_ERR

#endif