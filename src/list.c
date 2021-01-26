#include "builtin/list.h"

#include <stdlib.h>

static inline void write_list(b_vm *vm, b_obj_list *list, b_value value) {
  write_value_arr(vm, &list->items, value);
}

static inline b_obj_list *copy_list(b_vm *vm, b_obj_list *list, int start,
                                    int length) {
  b_obj_list *_list = new_list(vm);

  _list->items.values = &list->items.values[start];
  _list->items.count = length;
  _list->items.capacity = length;

  return _list;
}

DECLARE_LIST_METHOD(length) {
  ENFORCE_ARG_COUNT(length, 0);
  RETURN_NUMBER(AS_LIST(METHOD_OBJECT)->items.count);
}

DECLARE_LIST_METHOD(append) {
  ENFORCE_ARG_COUNT(append, 1);
  write_list(vm, AS_LIST(METHOD_OBJECT), args[0]);
  RETURN;
}

DECLARE_LIST_METHOD(clear) {
  ENFORCE_ARG_COUNT(clear, 0);
  free_value_arr(vm, &AS_LIST(METHOD_OBJECT)->items);
  RETURN;
}

DECLARE_LIST_METHOD(clone) {
  ENFORCE_ARG_COUNT(clone, 0);
  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  b_obj_list *new_list = copy_list(vm, list, 0, list->items.count);
  RETURN_OBJ(new_list);
}

DECLARE_LIST_METHOD(count) {
  ENFORCE_ARG_COUNT(count, 1);
  b_obj_list *list = AS_LIST(METHOD_OBJECT);

  int count = 0;
  for (int i = 0; i < list->items.count; i++) {
    if (values_equal(list->items.values[i], args[0]))
      count++;
  }

  RETURN_NUMBER(count);
}

DECLARE_LIST_METHOD(extend) {
  ENFORCE_ARG_COUNT(extend, 1);
  ENFORCE_ARG_TYPE(extend, 0, IS_LIST);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  b_obj_list *list2 = AS_LIST(args[0]);

  for (int i = 0; i < list2->items.count; i++) {
    write_list(vm, list, list2->items.values[i]);
  }

  RETURN;
}

DECLARE_LIST_METHOD(index_of) {
  ENFORCE_ARG_COUNT(index_of, 1);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);

  for (int i = 0; i < list->items.count; i++) {
    if (values_equal(list->items.values[i], args[0]))
      RETURN_NUMBER(i);
  }

  RETURN_NUMBER(-1);
}

DECLARE_LIST_METHOD(insert) {
  ENFORCE_ARG_COUNT(insert, 2);
  ENFORCE_ARG_TYPE(insert, 1, IS_NUMBER);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  int index = (int)AS_NUMBER(args[1]);

  insert_value_arr(vm, &list->items, args[0], index);
  RETURN;
}

DECLARE_LIST_METHOD(pop) {
  ENFORCE_ARG_COUNT(pop, 0);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  if (list->items.count > 0) {
    b_value value = list->items.values[list->items.count - 1]; // value to pop
    list->items.count--;
    RETURN_VALUE(value);
  }
  RETURN;
}

DECLARE_LIST_METHOD(shift) {
  ENFORCE_ARG_RANGE(shift, 0, 1);

  int count = 1;
  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(shift, 0, IS_NUMBER);
    count = AS_NUMBER(args[0]);
  }

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  if (count >= list->items.count || list->items.count == 1) {
    list->items.count = 0;
    RETURN;
  } else if (count > 0) {
    b_obj_list *nlist = new_list(vm);
    for (int i = 0; i < count; i++) {
      write_list(vm, nlist, list->items.values[0]);
      for (int j = 0; j < list->items.count; j++) {
        list->items.values[j] = list->items.values[j + 1];
      }
      list->items.count -= 1;
    }

    if (count == 1) {
      RETURN_VALUE(nlist->items.values[0]);
    } else {
      RETURN_OBJ(nlist);
    }
  }
  RETURN;
}

DECLARE_LIST_METHOD(remove_at) {
  ENFORCE_ARG_COUNT(remove_at, 1);
  ENFORCE_ARG_TYPE(remove_at, 0, IS_NUMBER);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  int index = AS_NUMBER(args[0]);
  if (index < 0 || index >= list->items.count) {
    RETURN_ERROR("list index %d out of range at remove_at()", index);
  }

  b_value value = list->items.values[index];
  for (int i = index; i < list->items.count; i++) {
    list->items.values[i] = list->items.values[i + 1];
  }
  list->items.count--;
  RETURN_VALUE(value);
}

DECLARE_LIST_METHOD(remove) {
  ENFORCE_ARG_COUNT(remove, 1);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  int index = -1;
  for (int i = 0; i < list->items.count; i++) {
    if (values_equal(list->items.values[i], args[0])) {
      index = i;
      break;
    }
  }

  if (index != -1) {
    for (int i = index; i < list->items.count; i++) {
      list->items.values[i] = list->items.values[i + 1];
    }
    list->items.count--;
  }
  RETURN;
}

DECLARE_LIST_METHOD(reverse) {
  ENFORCE_ARG_COUNT(reverse, 0);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);

  int start = 0, end = list->items.count - 1;
  while (start < end) {
    b_value temp = list->items.values[start];
    list->items.values[start] = list->items.values[end];
    list->items.values[end] = temp;
    start++;
    end--;
  }

  RETURN;
}

DECLARE_LIST_METHOD(sort) {
  ENFORCE_ARG_COUNT(sort, 0);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  sort_values(list->items.values, list->items.count);
  RETURN;
}

DECLARE_LIST_METHOD(contains) {
  ENFORCE_ARG_COUNT(contains, 1);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);

  for (int i = 0; i < list->items.count; i++) {
    if (values_equal(args[0], list->items.values[i]))
      RETURN_TRUE;
  }
  RETURN_FALSE;
}

DECLARE_LIST_METHOD(delete) {
  ENFORCE_ARG_RANGE(delete, 1, 2);
  ENFORCE_ARG_TYPE(delete, 0, IS_NUMBER);

  int lower_index = AS_NUMBER(args[0]);
  int upper_index = lower_index;

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(delete, 1, IS_NUMBER);
    upper_index = AS_NUMBER(args[1]);
  }

  b_obj_list *list = AS_LIST(METHOD_OBJECT);

  if (lower_index < 0 || lower_index >= list->items.count) {
    RETURN_ERROR("list index %d out of range at delete()", lower_index);
  } else if (upper_index < lower_index || upper_index >= list->items.count) {
    RETURN_ERROR("invalid upper limit %d at delete()", upper_index);
  }

  for (int i = 0; i < list->items.count - upper_index; i++) {
    list->items.values[lower_index + i] =
        list->items.values[i + upper_index + 1];
  }
  list->items.count -= upper_index - lower_index + 1;
  RETURN_NUMBER(upper_index - lower_index + 1);
}

DECLARE_LIST_METHOD(first) {
  ENFORCE_ARG_COUNT(first, 0);
  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  if (list->items.count > 0) {
    RETURN_VALUE(list->items.values[0]);
  } else {
    RETURN;
  }
}

DECLARE_LIST_METHOD(last) {
  ENFORCE_ARG_COUNT(last, 0);
  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  if (list->items.count > 0) {
    RETURN_VALUE(list->items.values[list->items.count - 1]);
  } else {
    RETURN;
  }
}

DECLARE_LIST_METHOD(is_empty) {
  ENFORCE_ARG_COUNT(is_empty, 0);
  RETURN_BOOL(AS_LIST(METHOD_OBJECT)->items.count == 0);
}

DECLARE_LIST_METHOD(take) {
  ENFORCE_ARG_COUNT(take, 1);
  ENFORCE_ARG_TYPE(take, 0, IS_NUMBER);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  int count = AS_NUMBER(args[0]);
  if (count < 0)
    count = list->items.count + count;

  if (list->items.count < count) {
    RETURN_OBJ(copy_list(vm, list, 0, list->items.count));
  }

  RETURN_OBJ(copy_list(vm, list, 0, count));
}

DECLARE_LIST_METHOD(get) {
  ENFORCE_ARG_COUNT(get, 1);
  ENFORCE_ARG_TYPE(get, 0, IS_NUMBER);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  int index = AS_NUMBER(args[0]);
  if (index < 0 || index >= list->items.count) {
    RETURN_ERROR("list index %d out of range at get()", index);
  }

  RETURN_VALUE(list->items.values[index]);
}

DECLARE_LIST_METHOD(compact) {
  ENFORCE_ARG_COUNT(compact, 0);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  b_obj_list *nlist = new_list(vm);

  for (int i = 0; i < list->items.count; i++) {
    if (!values_equal(list->items.values[i], NIL_VAL)) {
      write_list(vm, nlist, list->items.values[i]);
    }
  }

  RETURN_OBJ(nlist);
}

DECLARE_LIST_METHOD(unique) {
  ENFORCE_ARG_COUNT(unique, 0);

  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  b_obj_list *nlist = new_list(vm);

  for (int i = 0; i < list->items.count; i++) {
    bool found = false;
    for (int j = 0; j < nlist->items.count; j++) {
      if (values_equal(nlist->items.values[j], list->items.values[i])) {
        found = true;
        continue;
      }
    }

    if (!found) {
      write_list(vm, nlist, list->items.values[i]);
    }
  }

  RETURN_OBJ(nlist);
}

DECLARE_LIST_METHOD(zip) {
  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  b_obj_list *nlist = new_list(vm);

  b_obj_list *arg_list[arg_count];
  for (int i = 0; i < arg_count; i++) {
    ENFORCE_ARG_TYPE(zip, i, IS_LIST);
    arg_list[i] = AS_LIST(args[i]);
  }

  for (int i = 0; i < list->items.count; i++) {
    b_obj_list *a_list = new_list(vm);
    write_list(vm, a_list, list->items.values[i]); // item of main list

    for (int j = 0; j < arg_count; j++) { // item of argument lists
      if (i < arg_list[j]->items.count) {
        write_list(vm, a_list, arg_list[j]->items.values[i]);
      } else {
        write_list(vm, a_list, NIL_VAL);
      }
    }

    write_list(vm, nlist, OBJ_VAL(a_list));
  }

  RETURN_OBJ(nlist);
}

DECLARE_LIST_METHOD(to_dict) {
  ENFORCE_ARG_COUNT(to_dict, 0);

  b_obj_dict *dict = new_dict(vm);
  b_obj_list *list = AS_LIST(METHOD_OBJECT);
  for (int i = 0; i < list->items.count; i++) {
    dict_set_entry(vm, dict, NUMBER_VAL(i), list->items.values[i]);
  }
  RETURN_OBJ(dict);
}