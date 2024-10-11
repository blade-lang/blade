#ifndef BLADE_TABLE_H
#define BLADE_TABLE_H

#include "common.h"
#include "value.h"

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

void free_table(b_vm *vm, b_table *table);

bool table_set(b_vm *vm, b_table *table, b_value key, b_value value);

bool table_get(b_table *table, b_value key, b_value *value);

bool table_delete(b_table *table, b_value key);

void table_add_all(b_vm *vm, b_table *from, b_table *to);
void table_copy(b_vm *vm, b_table *from, b_table *to);

b_obj_string *table_find_string(b_table *table, const char *chars, int length,
                                uint32_t hash);

b_value table_find_key(b_table *table, b_value value);
b_obj_list *table_get_keys(b_vm *vm, b_table *table);

void table_print(b_table *table);

void mark_table(b_vm *vm, b_table *table);

void table_remove_whites(b_vm *vm, b_table *table);

void table_import_all(b_vm *vm, b_table *from, b_table *to);

#endif