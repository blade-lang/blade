#ifndef BLADE_MEMORY_H
#define BLADE_MEMORY_H

#include "common.h"
#include "vm.h"

#define GROW_CAPACITY(capacity) ((capacity) < 4 ? 4 : (capacity)*2)

#define GROW_ARRAY(type, pointer, old_count, new_count)                        \
  (type *)reallocate(vm, pointer, sizeof(type) * (old_count),                  \
                     sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count)                                   \
  reallocate(vm, pointer, sizeof(type) * (old_count), 0)

#define FREE(type, pointer) reallocate(vm, pointer, sizeof(type), 0)

#define ALLOCATE(type, count)                                                  \
  (type *)reallocate(vm, NULL, 0, sizeof(type) * (count))

#define C_ALLOCATE(type, count)                                                  \
  (type *)c_allocate(vm, sizeof(type), (count))

#define N_ALLOCATE(type, count)                                                  \
  (type *)allocate(vm, (count))

void *allocate(b_vm *vm, size_t size);
void *c_allocate(b_vm *vm, size_t size, size_t length);
void *reallocate(b_vm *vm, void *pointer, size_t old_size, size_t new_size);

void free_object(b_vm *vm, b_obj *object);
void free_objects(b_vm *vm);

void mark_object(b_vm *vm, b_obj *object);

void mark_value(b_vm *vm, b_value value);

void collect_garbage(b_vm *vm);

void blacken_object(b_vm *vm, b_obj *object);

#endif