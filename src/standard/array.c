#include "module.h"

typedef struct {
  void *buffer;
  int length;
} b_array;

void array_free(void *data) {
  if(data) {
    free(data);
  }
}

b_obj_ptr *new_array(b_vm *vm, b_array *array) {
  b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, array));
  ptr->free_fn = &array_free;
  return ptr;
}

//--------- INT 16 STARTS -------------------------
b_array *new_int16_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(int16_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__int16array) {
  ENFORCE_ARG_COUNT(int16array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_int16_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_int16_array(vm, list->items.count);
    int16_t *values = (int16_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("Int16Array() expects a list of valid int16");
      }

      values[i] = (int16_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or int16 list as argument");
}

DECLARE_MODULE_METHOD(array_int16_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(int16_t, array->buffer, array->length, array->length++);

    int16_t *values = (int16_t *)array->buffer;
    values[array->length - 1] = (int16_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(int16_t, array->buffer, array->length, array->length + list->items.count);

      int16_t *values = (int16_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("Int16Array lists can only contain numbers");
        }

        values[array->length + i] = (int16_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("Int16Array can only append an int16 or a list of int16");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_int16_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *data = (int16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int16Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_int16_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *data = (int16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int16Array index %d out of range", index);
  }

  int16_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_int16_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *data = (int16_t *)array->buffer;

  b_array *n_array = new_int16_array(vm, array->length);
  int16_t *n_data = (int16_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int16_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_int16_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int16_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t last = ((int16_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_int16_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *values = (int16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int16Array index %d out of range", index);
  }

  int16_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_int16_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *values = (int16_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_int16___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int16_t *values = (int16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- INT 32 STARTS -------------------------

b_array *new_int32_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(int32_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__int32array) {
  ENFORCE_ARG_COUNT(int32array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_int32_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_int32_array(vm, list->items.count);
    int32_t *values = (int32_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("Int32Array() expects a list of valid int32");
      }

      values[i] = (int32_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or int32 list as argument");
}

DECLARE_MODULE_METHOD(array_int32_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(int32_t, array->buffer, array->length, array->length++);

    int32_t *values = (int32_t *)array->buffer;
    values[array->length - 1] = (int32_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(int32_t, array->buffer, array->length, array->length + list->items.count);

      int32_t *values = (int32_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("Int32Array lists can only contain numbers");
        }

        values[array->length + i] = (int32_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("Int32Array can only append an int32 or a list of int32");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_int32_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *data = (int32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int32Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_int32_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *data = (int32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int32Array index %d out of range", index);
  }

  int32_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_int32_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *data = (int32_t *)array->buffer;

  b_array *n_array = new_int32_array(vm, array->length);
  int32_t *n_data = (int32_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int32_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_int32_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int32_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t last = ((int32_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_int32_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *values = (int32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int32Array index %d out of range", index);
  }

  int32_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_int32_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *values = (int32_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_int32___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int32_t *values = (int32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- INT 64 STARTS -------------------------

b_array *new_int64_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(int64_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__int64array) {
  ENFORCE_ARG_COUNT(int64array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_int64_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_int64_array(vm, list->items.count);
    int64_t *values = (int64_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("Int64Array() expects a list of valid int64");
      }

      values[i] = (int64_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or int64 list as argument");
}

DECLARE_MODULE_METHOD(array_int64_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(int64_t, array->buffer, array->length, array->length++);

    int64_t *values = (int64_t *)array->buffer;
    values[array->length - 1] = (int64_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(int64_t, array->buffer, array->length, array->length + list->items.count);

      int64_t *values = (int64_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("Int64Array lists can only contain numbers");
        }

        values[array->length + i] = (int64_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("Int64Array can only append an int64 or a list of int64");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_int64_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *data = (int64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int64Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_int64_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *data = (int64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int64Array index %d out of range", index);
  }

  int64_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_int64_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *data = (int64_t *)array->buffer;

  b_array *n_array = new_int64_array(vm, array->length);
  int64_t *n_data = (int64_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int64_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_int64_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_int64_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t last = ((int64_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_int64_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *values = (int64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int64Array index %d out of range", index);
  }

  int64_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_int64_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *values = (int64_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_int64___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int64_t *values = (int64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- Unsigned INT 16 STARTS ----------------

b_array *new_uint16_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(uint16_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__uint16array) {
  ENFORCE_ARG_COUNT(uint16array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_uint16_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_uint16_array(vm, list->items.count);
    uint16_t *values = (uint16_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("UInt16Array() expects a list of valid uint16");
      }

      values[i] = (uint16_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or uint16 list as argument");
}

DECLARE_MODULE_METHOD(array_uint16_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(uint16_t, array->buffer, array->length, array->length++);

    uint16_t *values = (uint16_t *)array->buffer;
    values[array->length - 1] = (uint16_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(uint16_t, array->buffer, array->length, array->length + list->items.count);

      uint16_t *values = (uint16_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("UInt16Array lists can only contain numbers");
        }

        values[array->length + i] = (uint16_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("UInt16Array can only append an uint16 or a list of uint16");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_uint16_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *data = (uint16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt16Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_uint16_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *data = (uint16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt16Array index %d out of range", index);
  }

  uint16_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_uint16_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *data = (uint16_t *)array->buffer;

  b_array *n_array = new_uint16_array(vm, array->length);
  uint16_t *n_data = (uint16_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint16_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_uint16_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint16_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t last = ((uint16_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_uint16_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *values = (uint16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt16Array index %d out of range", index);
  }

  uint16_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_uint16_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *values = (uint16_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_uint16___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint16_t *values = (uint16_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- Unsigned INT 32 STARTS ----------------

b_array *new_uint32_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(uint32_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__uint32array) {
  ENFORCE_ARG_COUNT(uint32array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_uint32_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_uint32_array(vm, list->items.count);
    uint32_t *values = (uint32_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("UInt32Array() expects a list of valid uint32");
      }

      values[i] = (uint32_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or uint32 list as argument");
}

DECLARE_MODULE_METHOD(array_uint32_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(uint32_t, array->buffer, array->length, array->length++);

    uint32_t *values = (uint32_t *)array->buffer;
    values[array->length - 1] = (uint32_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(uint32_t, array->buffer, array->length, array->length + list->items.count);

      uint32_t *values = (uint32_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("UInt32Array lists can only contain numbers");
        }

        values[array->length + i] = (uint32_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("UInt32Array can only append an uint32 or a list of uint32");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_uint32_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *data = (uint32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt32Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_uint32_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *data = (uint32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt32Array index %d out of range", index);
  }

  uint32_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_uint32_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *data = (uint32_t *)array->buffer;

  b_array *n_array = new_uint32_array(vm, array->length);
  uint32_t *n_data = (uint32_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint32_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_uint32_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint32_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t last = ((uint32_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_uint32_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *values = (uint32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt32Array index %d out of range", index);
  }

  uint32_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_uint32_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *values = (uint32_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_uint32___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint32_t *values = (uint32_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- Unsigned INT 64 STARTS ----------------

b_array *new_uint64_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(int64_t, length);
  return array;
}

DECLARE_MODULE_METHOD(array__uint64array) {
  ENFORCE_ARG_COUNT(uint32array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_uint64_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_uint64_array(vm, list->items.count);
    uint64_t *values = (uint64_t *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("UInt32Array() expects a list of valid uint64");
      }

      values[i] = (uint64_t) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or uint64 list as argument");
}

DECLARE_MODULE_METHOD(array_uint64_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(uint64_t, array->buffer, array->length, array->length++);

    uint64_t *values = (uint64_t *)array->buffer;
    values[array->length - 1] = (uint64_t) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(uint64_t , array->buffer, array->length, array->length + list->items.count);

      uint64_t *values = (uint64_t *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("UInt64Array lists can only contain numbers");
        }

        values[array->length + i] = (uint64_t) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("UInt64Array can only append an uint64 or a list of uint64");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_uint64_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *data = (uint64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt64Array index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_uint64_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *data = (uint64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt64Array index %d out of range", index);
  }

  uint64_t value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_uint64_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *data = (uint64_t *)array->buffer;

  b_array *n_array = new_uint64_array(vm, array->length);
  uint64_t *n_data = (uint64_t *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint64_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_uint64_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_uint64_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t last = ((uint64_t *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_uint64_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *values = (uint64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("UInt64Array index %d out of range", index);
  }

  uint64_t val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_uint64_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *values = (uint64_t *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_uint64___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  uint64_t *values = (uint64_t *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- FLOAT STARTS -------------------------

b_array *new_float_array(b_vm *vm, int length) {
  b_array *array = ALLOCATE(b_array, 1);
  array->length = length;
  array->buffer = ALLOCATE(float, length);
  return array;
}

DECLARE_MODULE_METHOD(array__floatarray) {
  ENFORCE_ARG_COUNT(int64array, 1);
  if (IS_NUMBER(args[0])) {
    RETURN_OBJ(new_array(vm, new_float_array(vm, (int) AS_NUMBER(args[0]))));
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_array *array = new_float_array(vm, list->items.count);
    float *values = (float *)array->buffer;

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("FloatArray() expects a list of valid int64");
      }

      values[i] = (float) AS_NUMBER(list->items.values[i]);
    }

    RETURN_OBJ(new_array(vm, array));
  }

  RETURN_ERROR("expected array size or float list as argument");
}

DECLARE_MODULE_METHOD(array_float_append) {
  ENFORCE_ARG_COUNT(append, 2);
  ENFORCE_ARG_TYPE(append, 0, IS_PTR);
  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  if (IS_NUMBER(args[1])) {

    array->buffer = GROW_ARRAY(float, array->buffer, array->length, array->length++);

    float *values = (float *)array->buffer;
    values[array->length - 1] = (float) AS_NUMBER(args[1]);

  } else if (IS_LIST(args[1])) {
    b_obj_list *list = AS_LIST(args[1]);
    if (list->items.count > 0) {

      array->buffer = GROW_ARRAY(float, array->buffer, array->length, array->length + list->items.count);

      float *values = (float *)array->buffer;

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("FloatArray lists can only contain numbers");
        }

        values[array->length + i] = (float) AS_NUMBER(list->items.values[i]);
      }

      array->length += list->items.count;
    }
  } else {
    RETURN_ERROR("FloatArray can only append an float or a list of float");
  }

  RETURN;
}

DECLARE_MODULE_METHOD(array_float_get) {
  ENFORCE_ARG_COUNT(get, 2);
  ENFORCE_ARG_TYPE(get, 0, IS_PTR);
  ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *data = (float *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("FloatArray index %d out of range", index);
  }

  RETURN_NUMBER((double) data[index]);
}

DECLARE_MODULE_METHOD(array_float_set) {
  ENFORCE_ARG_COUNT(set, 3);
  ENFORCE_ARG_TYPE(set, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(set, 2, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *data = (float *)array->buffer;

  int index = AS_NUMBER(args[1]);
  if (index < 0 || index >= array->length) {
    RETURN_ERROR("FloatArray index %d out of range", index);
  }

  float value = AS_NUMBER(args[2]);
  data[index] = value;

  RETURN_NUMBER((double) value);
}

DECLARE_MODULE_METHOD(array_float_reverse) {
  ENFORCE_ARG_COUNT(reverse, 1);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *data = (float *)array->buffer;

  b_array *n_array = new_float_array(vm, array->length);
  float *n_data = (float *)n_array->buffer;

  for (int i = array->length - 1; i >= 0; i--) {
    n_data[i] = data[i];
  }

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_float_clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;

  b_array *n_array = new_float_array(vm, array->length);
  memcpy(n_array->buffer, array->buffer, array->length);

  RETURN_OBJ(new_array(vm, n_array));
}

DECLARE_MODULE_METHOD(array_float_pop) {
  ENFORCE_ARG_COUNT(pop, 1);
  ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float last = ((float *)array->buffer)[array->length - 1];
  array->length--;

  RETURN_NUMBER(last);
}

DECLARE_MODULE_METHOD(array_float_remove) {
  ENFORCE_ARG_COUNT(remove, 2);
  ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
  ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *values = (float *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index < 0 || index >= array->length) {
    RETURN_ERROR("Int64Array index %d out of range", index);
  }

  float val = values[index];

  for (int i = index; i < array->length; i++) {
    values[i] = values[i + 1];
  }
  array->length--;

  RETURN_NUMBER(val);
}

DECLARE_MODULE_METHOD(array_float_to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *values = (float *)array->buffer;

  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < array->length; i++) {
    write_list(vm, list, NUMBER_VAL((double)values[i]));
  }

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(array_float___iter__) {
  ENFORCE_ARG_COUNT(@iter, 2);
  ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  float *values = (float *)array->buffer;

  int index = AS_NUMBER(args[1]);

  if (index > -1 && index < array->length) {
    RETURN_NUMBER(values[index]);
  }

  RETURN_NIL;
}


//--------- COMMON STARTS -------------------------

DECLARE_MODULE_METHOD(array_length) {
  ENFORCE_ARG_COUNT(length, 1);
  ENFORCE_ARG_TYPE(length, 0, IS_PTR);

  b_obj_ptr *ptr = AS_PTR(args[0]);
  RETURN_NUMBER(((b_array *)ptr->pointer)->length);
}

DECLARE_MODULE_METHOD(array_first) {
  ENFORCE_ARG_COUNT(first, 1);
  ENFORCE_ARG_TYPE(first, 0, IS_PTR);
  RETURN_NUMBER(((double *)((b_array *) AS_PTR(args[0])->pointer)->buffer)[0]);
}

DECLARE_MODULE_METHOD(array_last) {
  ENFORCE_ARG_COUNT(first, 1);
  ENFORCE_ARG_TYPE(first, 0, IS_PTR);
  b_array *array = (b_array *) AS_PTR(args[0])->pointer;
  RETURN_NUMBER(((double *)array->buffer)[array->length - 1]);
}

DECLARE_MODULE_METHOD(array_extend) {
  ENFORCE_ARG_COUNT(extend, 2);
  ENFORCE_ARG_TYPE(extend, 0, IS_PTR);
  ENFORCE_ARG_TYPE(extend, 1, IS_PTR);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  b_array  *array2 = (b_array *) AS_PTR(args[1])->pointer;

  array->buffer = GROW_ARRAY(void, array->buffer, array->length, array->length + array2->length);

  memcpy(array->buffer + array->length, array2->buffer, array2->length);
  array->length += array2->length;
  RETURN;
}

DECLARE_MODULE_METHOD(array_to_string) {
  ENFORCE_ARG_COUNT(to_string, 2);
  ENFORCE_ARG_TYPE(to_string, 0, IS_PTR);
  ENFORCE_ARG_TYPE(to_bytes, 1, IS_NUMBER);

  int size = AS_NUMBER(args[1]);
  b_array *array = (b_array *)AS_PTR(args[0])->pointer;
  RETURN_L_STRING(array->buffer, array->length * size);
}

DECLARE_MODULE_METHOD(array_to_bytes) {
  ENFORCE_ARG_COUNT(to_bytes, 2);
  ENFORCE_ARG_TYPE(to_bytes, 0, IS_PTR);
  ENFORCE_ARG_TYPE(to_bytes, 1, IS_NUMBER);

  b_array  *array = (b_array *) AS_PTR(args[0])->pointer;
  int size = AS_NUMBER(args[1]);

  b_obj_bytes *bytes = (b_obj_bytes *)GC(new_bytes(vm, array->length * size));
  memcpy(bytes->bytes.bytes, array->buffer, array->length * size);

  RETURN_OBJ(bytes);
}

DECLARE_MODULE_METHOD(array___itern__) {
  ENFORCE_ARG_COUNT(@itern, 2);
  ENFORCE_ARG_TYPE(@itern, 0, IS_PTR);
  ENFORCE_ARG_TYPE(@itern, 1, IS_NUMBER);

  b_array *array = (b_array *)AS_PTR(args[0])->pointer;

  if (IS_NIL(args[1])) {
    if (array->length == 0) RETURN_FALSE;
    RETURN_NUMBER(0);
  }

  if (!IS_NUMBER(args[1])) {
    RETURN_ERROR("Arrays are numerically indexed");
  }

  int index = AS_NUMBER(args[0]);
  if (index < array->length - 1) {
    RETURN_NUMBER((double) index + 1);
  }

  RETURN_NIL;
}

CREATE_MODULE_LOADER(array) {
  static b_func_reg module_functions[] = {
      // int16
      {"Int16Array", false, GET_MODULE_METHOD(array__int16array)},
      {"int16_append", false, GET_MODULE_METHOD(array_int16_append)},
      {"int16_get", false, GET_MODULE_METHOD(array_int16_get)},
      {"int16_set", false, GET_MODULE_METHOD(array_int16_set)},
      {"int16_reverse", false, GET_MODULE_METHOD(array_int16_reverse)},
      {"int16_clone", false, GET_MODULE_METHOD(array_int16_clone)},
      {"int16_pop", false, GET_MODULE_METHOD(array_int16_pop)},
      {"int16_to_list", false, GET_MODULE_METHOD(array_int16_to_list)},
      {"int16___iter__", false, GET_MODULE_METHOD(array_int16___iter__)},

      // int32
      {"Int32Array", false, GET_MODULE_METHOD(array__int32array)},
      {"int32_append", false, GET_MODULE_METHOD(array_int32_append)},
      {"int32_get", false, GET_MODULE_METHOD(array_int32_get)},
      {"int32_set", false, GET_MODULE_METHOD(array_int32_set)},
      {"int32_reverse", false, GET_MODULE_METHOD(array_int32_reverse)},
      {"int32_clone", false, GET_MODULE_METHOD(array_int32_clone)},
      {"int32_pop", false, GET_MODULE_METHOD(array_int32_pop)},
      {"int32_to_list", false, GET_MODULE_METHOD(array_int32_to_list)},
      {"int32___iter__", false, GET_MODULE_METHOD(array_int32___iter__)},

      // int64
      {"Int64Array", false, GET_MODULE_METHOD(array__int64array)},
      {"int64_append", false, GET_MODULE_METHOD(array_int64_append)},
      {"int64_get", false, GET_MODULE_METHOD(array_int64_get)},
      {"int64_set", false, GET_MODULE_METHOD(array_int64_set)},
      {"int64_reverse", false, GET_MODULE_METHOD(array_int64_reverse)},
      {"int64_clone", false, GET_MODULE_METHOD(array_int64_clone)},
      {"int64_pop", false, GET_MODULE_METHOD(array_int64_pop)},
      {"int64_to_list", false, GET_MODULE_METHOD(array_int64_to_list)},
      {"int64___iter__", false, GET_MODULE_METHOD(array_int64___iter__)},

      // uint16
      {"UInt16Array", false, GET_MODULE_METHOD(array__uint16array)},
      {"uint16_append", false, GET_MODULE_METHOD(array_uint16_append)},
      {"uint16_get", false, GET_MODULE_METHOD(array_uint16_get)},
      {"uint16_set", false, GET_MODULE_METHOD(array_uint16_set)},
      {"uint16_reverse", false, GET_MODULE_METHOD(array_uint16_reverse)},
      {"uint16_clone", false, GET_MODULE_METHOD(array_uint16_clone)},
      {"uint16_pop", false, GET_MODULE_METHOD(array_uint16_pop)},
      {"uint16_to_list", false, GET_MODULE_METHOD(array_uint16_to_list)},
      {"uint16___iter__", false, GET_MODULE_METHOD(array_uint16___iter__)},

      // uint32
      {"UInt32Array", false, GET_MODULE_METHOD(array__uint32array)},
      {"uint32_append", false, GET_MODULE_METHOD(array_uint32_append)},
      {"uint32_get", false, GET_MODULE_METHOD(array_uint32_get)},
      {"uint32_set", false, GET_MODULE_METHOD(array_uint32_set)},
      {"uint32_reverse", false, GET_MODULE_METHOD(array_uint32_reverse)},
      {"uint32_clone", false, GET_MODULE_METHOD(array_uint32_clone)},
      {"uint32_pop", false, GET_MODULE_METHOD(array_uint32_pop)},
      {"uint32_to_list", false, GET_MODULE_METHOD(array_uint32_to_list)},
      {"uint32___iter__", false, GET_MODULE_METHOD(array_uint32___iter__)},

      // uint64
      {"UInt64Array", false, GET_MODULE_METHOD(array__uint64array)},
      {"uint64_append", false, GET_MODULE_METHOD(array_uint64_append)},
      {"uint64_get", false, GET_MODULE_METHOD(array_uint64_get)},
      {"uint64_set", false, GET_MODULE_METHOD(array_uint64_set)},
      {"uint64_reverse", false, GET_MODULE_METHOD(array_uint64_reverse)},
      {"uint64_clone", false, GET_MODULE_METHOD(array_uint64_clone)},
      {"uint64_pop", false, GET_MODULE_METHOD(array_uint64_pop)},
      {"uint64_to_list", false, GET_MODULE_METHOD(array_uint64_to_list)},
      {"uint64___iter__", false, GET_MODULE_METHOD(array_uint64___iter__)},

      // float
      {"FloatArray", false, GET_MODULE_METHOD(array__floatarray)},
      {"float_append", false, GET_MODULE_METHOD(array_float_append)},
      {"float_get", false, GET_MODULE_METHOD(array_float_get)},
      {"float_set", false, GET_MODULE_METHOD(array_float_set)},
      {"float_reverse", false, GET_MODULE_METHOD(array_float_reverse)},
      {"float_clone", false, GET_MODULE_METHOD(array_float_clone)},
      {"float_pop", false, GET_MODULE_METHOD(array_float_pop)},
      {"float_to_list", false, GET_MODULE_METHOD(array_float_to_list)},
      {"float___iter__", false, GET_MODULE_METHOD(array_float___iter__)},

      // common
      {"length", false, GET_MODULE_METHOD(array_length)},
      {"first", false, GET_MODULE_METHOD(array_first)},
      {"last", false, GET_MODULE_METHOD(array_last)},
      {"extend", false, GET_MODULE_METHOD(array_extend)},
      {"to_string", false, GET_MODULE_METHOD(array_to_string)},
      {"to_bytes", false, GET_MODULE_METHOD(array_to_bytes)},
      {"itern", false, GET_MODULE_METHOD(array___itern__)},
      {NULL,     false, NULL},
  };

  static b_module_reg module = {
      .name = "_array",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}