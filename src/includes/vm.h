#ifndef bird_vm_h
#define bird_vm_h

#include "blob.h"
#include "config.h"
#include "table.h"
#include "value.h"

typedef enum {
  PTR_OK,
  PTR_COMPILE_ERR,
  PTR_RUNTIME_ERR,
} b_ptr_result;

typedef struct {
  b_blob *blob;
  uint8_t *ip;
  b_value stack[STACK_MAX];
  b_value *stack_top;
  b_table strings;
  b_table globals;

  b_obj *objects;
} b_vm;

void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
b_ptr_result interpret(b_vm *vm, const char *source);
void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);

#endif