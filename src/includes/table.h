#ifndef bird_table_h
#define bird_table_h

#include "common.h"
#include "value.h"
#include "vm.h"

typedef struct {
  b_value key;
  b_value value;
} b_entry;

typedef struct {
  int count;
  int capacity;
  b_entry *entries;
} b_table;

void init_table(b_table *table);
void free_table(b_table *table);
bool table_set(b_table *table, b_value key, b_value value);
bool table_get(b_table *table, b_value key, b_value *value);
bool table_delete(b_table *table, b_value key);
void table_add_all(b_table *from, b_table *to);
b_obj_string *table_find_string(b_table *table, const char *chars, int length,
                                uint32_t hash);
void table_print(b_table *table);
void mark_table(b_vm *vm, b_table *table);
void table_remove_whites(b_table *table);

#endif