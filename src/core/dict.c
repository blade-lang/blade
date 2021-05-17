#include "builtin/dict.h"

#include <stdlib.h>

#define ENFORCE_VALID_DICT_KEY(name, index)                                    \
  EXCLUDE_ARG_TYPE(name, IS_LIST, index);                                      \
  EXCLUDE_ARG_TYPE(name, IS_DICT, index);

DECLARE_DICT_METHOD(length) {
  ENFORCE_ARG_COUNT(dictionary.length, 0);
  RETURN_NUMBER(AS_DICT(METHOD_OBJECT)->names.count);
}

DECLARE_DICT_METHOD(add) {
  ENFORCE_ARG_COUNT(add, 2);
  ENFORCE_VALID_DICT_KEY(add, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);

  b_value temp_value;
  if (table_get(&dict->items, args[0], &temp_value)) {
    RETURN_ERROR("duplicate key %s at add()", value_to_string(vm, args[0]));
  }

  dict_add_entry(vm, dict, args[0], args[1]);
  RETURN;
}

DECLARE_DICT_METHOD(clear) {
  ENFORCE_ARG_COUNT(dict, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  free_value_arr(vm, &dict->names);
  free_table(vm, &dict->items);
  RETURN;
}

DECLARE_DICT_METHOD(clone) {
  ENFORCE_ARG_COUNT(clone, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_dict *n_dict = new_dict(vm);

  table_add_all(vm, &dict->items, &n_dict->items);

  for (int i = 0; i < dict->names.count; i++) {
    write_value_arr(vm, &n_dict->names, dict->names.values[i]);
  }

  RETURN_OBJ(n_dict);
}

DECLARE_DICT_METHOD(compact) {
  ENFORCE_ARG_COUNT(compact, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_dict *n_dict = new_dict(vm);
  push(vm, OBJ_VAL(n_dict)); // looking at gc

  for (int i = 0; i < dict->names.count; i++) {
    b_value tmp_value;
    table_get(&dict->items, dict->names.values[i], &tmp_value);
    if (!values_equal(tmp_value, NIL_VAL)) {
      dict_add_entry(vm, n_dict, dict->names.values[i], tmp_value);
    }
  }

  pop(vm); // looking at gc
  RETURN_OBJ(n_dict);
}

DECLARE_DICT_METHOD(contains) {
  ENFORCE_ARG_COUNT(contains, 1);
  ENFORCE_VALID_DICT_KEY(contains, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_value value;
  RETURN_BOOL(table_get(&dict->items, args[0], &value));
}

DECLARE_DICT_METHOD(extend) {
  ENFORCE_ARG_COUNT(extend, 1);
  ENFORCE_ARG_TYPE(extend, 0, IS_DICT);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_dict *dict_cpy = AS_DICT(args[0]);

  for (int i = 0; i < dict_cpy->names.count; i++) {
    write_value_arr(vm, &dict->names, dict_cpy->names.values[i]);
  }
  table_add_all(vm, &dict_cpy->items, &dict->items);
  RETURN;
}

DECLARE_DICT_METHOD(get) {
  ENFORCE_ARG_RANGE(get, 1, 2);
  ENFORCE_VALID_DICT_KEY(get, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_value value;
  if (!dict_get_entry(dict, args[0], &value)) {
    if (arg_count == 1) {
      RETURN_ERROR("invalid key %s in get()", value_to_string(vm, args[0]));
    } else {
      RETURN_VALUE(args[1]); // return default
    }
  }

  RETURN_VALUE(value);
}

DECLARE_DICT_METHOD(keys) {
  ENFORCE_ARG_COUNT(keys, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_list *list = new_list(vm);
  for (int i = 0; i < dict->names.count; i++) {
    write_list(vm, list, dict->names.values[i]);
  }
  RETURN_OBJ(list);
}

DECLARE_DICT_METHOD(values) {
  ENFORCE_ARG_COUNT(values, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_list *list = new_list(vm);
  for (int i = 0; i < dict->names.count; i++) {
    b_value tmp_value;
    dict_get_entry(dict, dict->names.values[i], &tmp_value);
    write_list(vm, list, tmp_value);
  }
  RETURN_OBJ(list);
}

DECLARE_DICT_METHOD(remove) {
  ENFORCE_ARG_COUNT(remove, 1);
  ENFORCE_VALID_DICT_KEY(remove, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_value value;
  if (table_get(&dict->items, args[0], &value)) {
    table_delete(&dict->items, args[0]);
    int index = -1;
    for (int i = 0; i < dict->names.count; i++) {
      if (values_equal(dict->names.values[i], args[0])) {
        index = i;
        break;
      }
    }

    for (int i = index; i < dict->names.count; i++) {
      dict->names.values[i] = dict->names.values[i + 1];
    }
    dict->names.count--;
    RETURN_VALUE(value);
  }
  RETURN;
}

DECLARE_DICT_METHOD(assign) {
  ENFORCE_ARG_COUNT(assign, 2);
  ENFORCE_VALID_DICT_KEY(assign, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_value value;
  if (!table_get(&dict->items, args[0], &value)) {
    dict_add_entry(vm, dict, args[0], args[1]);
  } else {
    dict_set_entry(vm, dict, args[0], args[1]);
  }
  RETURN;
}

DECLARE_DICT_METHOD(is_empty) {
  ENFORCE_ARG_COUNT(is_empty, 0);
  RETURN_BOOL(AS_DICT(METHOD_OBJECT)->names.count == 0);
}

DECLARE_DICT_METHOD(find_key) {
  ENFORCE_ARG_COUNT(find_key, 1);
  RETURN_VALUE(table_find_key(&AS_DICT(METHOD_OBJECT)->items, args[0]));
}

DECLARE_DICT_METHOD(to_list) {
  ENFORCE_ARG_COUNT(to_list, 0);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_list *name_list = new_list(vm);
  b_obj_list *value_list = new_list(vm);
  for (int i = 0; i < dict->names.count; i++) {
    write_list(vm, name_list, dict->names.values[i]);
    b_value value;
    if (table_get(&dict->items, dict->names.values[i], &value)) {
      write_list(vm, value_list, value);
    } else { // theoretically impossible
      write_list(vm, value_list, NIL_VAL);
    }
  }

  b_obj_list *list = new_list(vm);
  write_list(vm, list, OBJ_VAL(name_list));
  write_list(vm, list, OBJ_VAL(value_list));

  RETURN_OBJ(list);
}

DECLARE_DICT_METHOD(has_attr) {
  ENFORCE_ARG_COUNT(has_attr, 1);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  for (int i = 0; i < dict->names.count; i++) {
    if (values_equal(dict->names.values[i], args[0]))
      RETURN_TRUE;
  }
  RETURN_FALSE;
}

DECLARE_DICT_METHOD(__iter__) {
  ENFORCE_ARG_COUNT(__iter__, 1);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);

  b_value result;
  if (table_get(&dict->items, args[0], &result)) {
    RETURN_VALUE(result);
  }

  RETURN;
}

DECLARE_DICT_METHOD(__itern__) {
  ENFORCE_ARG_COUNT(__itern__, 1);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);

  if (IS_NIL(args[0])) {
    if (dict->names.count == 0)
      RETURN_FALSE;
    RETURN_VALUE(dict->names.values[0]);
  }
  for (int i = 0; i < dict->names.count; i++) {
    if (values_equal(args[0], dict->names.values[i]) &&
        (i + 1) < dict->names.count) {
      RETURN_VALUE(dict->names.values[i + 1]);
    }
  }

  RETURN;
}

#undef ENFORCE_VALID_DICT_KEY