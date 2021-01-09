#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

void init_table(b_table *table) {
  table->capacity = 0;
  table->count = 0;
  table->entries = NULL;
}

void free_table(b_table *table) {
  FREE_ARRAY(b_entry, table->entries, table->capacity);
  init_table(table);
}

static b_entry *find_entry(b_entry *entries, int capacity, b_value key) {
  uint32_t index = hash_value(key) % capacity;
  b_entry *tombstone = NULL;

  for (;;) {
    b_entry *entry = &entries[index];

    if (IS_EMPTY(entry->key)) {
      if (IS_NIL(entry->value)) {
        // empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {
        // we found a tombstone
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (values_equal(entry->key, key)) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjust_capacity(b_table *table, int capacity) {
  b_entry *entries = ALLOCATE(b_entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = EMPTY_VAL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];

    if (IS_EMPTY(entry->key))
      continue;

    b_entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(b_entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(b_table *table, b_value key, b_value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  b_entry *entry = find_entry(table->entries, table->capacity, key);

  bool is_new_key = IS_EMPTY(entry->key);
  if (is_new_key && IS_NIL(entry->value))
    table->count++;

  entry->key = key;
  entry->value = value;

  return is_new_key;
}

bool table_get(b_table *table, b_value key, b_value *value) {
  if (table->count == 0)
    return false;

  b_entry *entry = find_entry(table->entries, table->count, key);
  if (IS_NIL(entry->key))
    return false;

  *value = entry->value;
  return true;
}

bool table_delete(b_table *table, b_value key) {
  if (table->count == 0)
    return false;

  // find the entry
  b_entry *entry = find_entry(table->entries, table->capacity, key);
  if (IS_EMPTY(entry->key))
    return false;

  // place a tombstone
  entry->key = EMPTY_VAL;
  entry->value = BOOL_VAL(true);

  return true;
}

void table_add_all(b_table *from, b_table *to) {
  for (int i = 0; i < from->capacity; i++) {
    b_entry *entry = &from->entries[i];
    if (!IS_EMPTY(entry->key)) {
      table_set(to, entry->key, entry->value);
    }
  }
}

b_obj_string *table_find_string(b_table *table, const char *chars, int length,
                                uint32_t hash) {
  if (table->count == 0)
    return NULL;

  uint32_t index = hash % table->capacity;

  for (;;) {
    b_entry *entry = &table->entries[index];

    if (IS_EMPTY(entry->key)) {
      // stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value))
        return NULL;
    }

    if (IS_STRING(entry->key)) {
      b_obj_string *string = AS_STRING(entry->key);
      if (string->length == length && string->hash == hash &&
          memcmp(string->chars, chars, length) == 0) {
        // we found it
        return string;
      }
    }

    index = (index + 1) % table->capacity;
  }
}