#ifndef bird_memory_h
#define bird_memory_h

#include "common.h"
#include "vm.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity * 2))

#define GROW_ARRAY(type, pointer, old_count, new_count)                        \
  (type *)reallocate(pointer, sizeof(type) * (old_count),                      \
                     sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count)                                   \
  reallocate(pointer, sizeof(type) * (old_count), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define ALLOCATE(type, count)                                                  \
  (type *)reallocate(NULL, 0, sizeof(type) * (count))

void *reallocate(void *pointer, size_t old_size, size_t new_size);
void free_objects(b_vm *vm);

#endif