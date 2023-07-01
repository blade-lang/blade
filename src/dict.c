#include "dict.h"

#include <stdlib.h>

#define ENFORCE_VALID_DICT_KEY(name, index)                                    \
  EXCLUDE_ARG_TYPE(name, IS_LIST, index);                                      \
  EXCLUDE_ARG_TYPE(name, IS_DICT, index); \
  EXCLUDE_ARG_TYPE(name, IS_FILE, index);

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
    RETURN_ERROR("duplicate key %s at add()", value_to_string(vm, args[0])->chars);
  }

  dict_add_entry(vm, dict, args[0], args[1]);
  RETURN;
}

DECLARE_DICT_METHOD(set) {
    ENFORCE_ARG_COUNT(set, 2);
    ENFORCE_VALID_DICT_KEY(set, 0);

    b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
    b_value value;
    if (!table_get(&dict->items, args[0], &value)) {
        dict_add_entry(vm, dict, args[0], args[1]);
    } else {
        dict_set_entry(vm, dict, args[0], args[1]);
    }
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
  b_obj_dict *n_dict = (b_obj_dict *) GC(new_dict(vm));

  table_add_all(vm, &dict->items, &n_dict->items);

  for (int i = 0; i < dict->names.count; i++) {
    write_value_arr(vm, &n_dict->names, dict->names.values[i]);
  }

  RETURN_OBJ(n_dict);
}

DECLARE_DICT_METHOD(compact) {
  ENFORCE_ARG_COUNT(compact, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_dict *n_dict = (b_obj_dict *) GC(new_dict(vm));

  for (int i = 0; i < dict->names.count; i++) {
    b_value tmp_value;
    table_get(&dict->items, dict->names.values[i], &tmp_value);
    if (!values_equal(tmp_value, NIL_VAL)) {
      dict_add_entry(vm, n_dict, dict->names.values[i], tmp_value);
    }
  }

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
    b_value tmp;
    if(!table_get(&dict->items, dict_cpy->names.values[i], &tmp)) {
      write_value_arr(vm, &dict->names, dict_cpy->names.values[i]);
    }
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
      RETURN_NIL;
    } else {
      RETURN_VALUE(args[1]); // return default
    }
  }

  RETURN_VALUE(value);
}

DECLARE_DICT_METHOD(keys) {
  ENFORCE_ARG_COUNT(keys, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_list *list = (b_obj_list *) GC(new_list(vm));
  for (int i = 0; i < dict->names.count; i++) {
    write_list(vm, list, dict->names.values[i]);
  }
  RETURN_OBJ(list);
}

DECLARE_DICT_METHOD(values) {
  ENFORCE_ARG_COUNT(values, 0);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_list *list = (b_obj_list *) GC(new_list(vm));
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
  RETURN_NIL;
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
  b_obj_list *name_list = (b_obj_list *) GC(new_list(vm));
  b_obj_list *value_list = (b_obj_list *) GC(new_list(vm));
  for (int i = 0; i < dict->names.count; i++) {
    write_list(vm, name_list, dict->names.values[i]);
    b_value value;
    if (table_get(&dict->items, dict->names.values[i], &value)) {
      write_list(vm, value_list, value);
    } else { // theoretically impossible
      write_list(vm, value_list, NIL_VAL);
    }
  }

  b_obj_list *list = (b_obj_list *) GC(new_list(vm));
  write_list(vm, list, OBJ_VAL(name_list));
  write_list(vm, list, OBJ_VAL(value_list));

  RETURN_OBJ(list);
}

DECLARE_DICT_METHOD(__iter__) {
  ENFORCE_ARG_COUNT(__iter__, 1);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);

  b_value result;
  if (table_get(&dict->items, args[0], &result)) {
    RETURN_VALUE(result);
  }

  RETURN_NIL;
}

DECLARE_DICT_METHOD(__itern__) {
  ENFORCE_ARG_COUNT(__itern__, 1);
  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);

  if (IS_NIL(args[0])) {
    if (dict->names.count == 0) RETURN_FALSE;
    RETURN_VALUE(dict->names.values[0]);
  }
  for (int i = 0; i < dict->names.count; i++) {
    if (values_equal(args[0], dict->names.values[i]) &&
        (i + 1) < dict->names.count) {
      RETURN_VALUE(dict->names.values[i + 1]);
    }
  }

  RETURN_NIL;
}


DECLARE_DICT_METHOD(each) {
  ENFORCE_ARG_COUNT(each, 1);
  ENFORCE_ARG_TYPE(each, 0, IS_CLOSURE);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
  b_obj_closure *closure = AS_CLOSURE(args[0]);

  b_obj_list *call_list = new_list(vm);
  push(vm, OBJ_VAL(call_list));

  ITER_TOOL_PREPARE();

  for(int i = 0; i < dict->names.count; i++) {
    if(arity > 0) {
      b_value value;
      table_get(&dict->items, dict->names.values[i], &value);

      call_list->items.values[0] = value;
      if(arity > 1) {
        call_list->items.values[1] = dict->names.values[i];
      }
    }

    call_closure(vm, closure, call_list);
  }

  pop(vm); // pop the argument list
  RETURN;
}

DECLARE_DICT_METHOD(filter) {
    ENFORCE_ARG_COUNT(filter, 1);
    ENFORCE_ARG_TYPE(filter, 0, IS_CLOSURE);

  b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
    b_obj_closure *closure = AS_CLOSURE(args[0]);

    b_obj_list *call_list = new_list(vm);
    push(vm, OBJ_VAL(call_list));

    ITER_TOOL_PREPARE();

    b_obj_dict *result_dict = (b_obj_dict *)GC(new_dict(vm));

    for(int i = 0; i < dict->names.count; i++) {
      b_value value;
      table_get(&dict->items, dict->names.values[i], &value);

      if(arity > 0) {
        call_list->items.values[0] = value;
        if(arity > 1) {
          call_list->items.values[1] = dict->names.values[i];
        }
      }

      b_value result = call_closure(vm, closure, call_list);
      if(!is_false(result)) {
        dict_add_entry(vm, result_dict, dict->names.values[i], value);
      }
    }

    pop(vm); // pop the call list
    RETURN_OBJ(result_dict);
}

DECLARE_DICT_METHOD(some) {
    ENFORCE_ARG_COUNT(some, 1);
    ENFORCE_ARG_TYPE(some, 0, IS_CLOSURE);

    b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
    b_obj_closure *closure = AS_CLOSURE(args[0]);

    b_obj_list *call_list = new_list(vm);
    push(vm, OBJ_VAL(call_list));

    ITER_TOOL_PREPARE();

    for(int i = 0; i < dict->names.count; i++) {
      if(arity > 0) {
        b_value value;
        table_get(&dict->items, dict->names.values[i], &value);
        call_list->items.values[0] = value;

        if(arity > 1) {
          call_list->items.values[1] = dict->names.values[i];
        }
      }

      b_value result = call_closure(vm, closure, call_list);
      if(!is_false(result)) {
        pop(vm); // pop the call list
        RETURN_TRUE;
      }
    }

    pop(vm); // pop the call list
    RETURN_FALSE;
}

DECLARE_DICT_METHOD(every) {
    ENFORCE_ARG_COUNT(every, 1);
    ENFORCE_ARG_TYPE(every, 0, IS_CLOSURE);

    b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
    b_obj_closure *closure = AS_CLOSURE(args[0]);

    b_obj_list *call_list = new_list(vm);
    push(vm, OBJ_VAL(call_list));

    ITER_TOOL_PREPARE();

    for(int i = 0; i < dict->names.count; i++) {
      if(arity > 0) {
        b_value value;
        table_get(&dict->items, dict->names.values[i], &value);
        call_list->items.values[0] = value;

        if(arity > 1) {
          call_list->items.values[1] = dict->names.values[i];
        }
      }

      b_value result = call_closure(vm, closure, call_list);
      if(is_false(result)) {
        pop(vm); // pop the call list
        RETURN_FALSE;
      }
    }

    pop(vm); // pop the call list
    RETURN_TRUE;
}

DECLARE_DICT_METHOD(reduce) {
    ENFORCE_ARG_RANGE(reduce, 1, 2);
    ENFORCE_ARG_TYPE(reduce, 0, IS_CLOSURE);

    b_obj_dict *dict = AS_DICT(METHOD_OBJECT);
    b_obj_closure *closure = AS_CLOSURE(args[0]);

    int start_index = 0;

    b_value accumulator = NIL_VAL;
    if(arg_count == 2) {
      accumulator = args[1];
    }

    if(IS_NIL(accumulator) && dict->names.count > 0) {
      table_get(&dict->items, dict->names.values[0], &accumulator);
      start_index = 1;
    }

    b_obj_list *call_list = new_list(vm);
    push(vm, OBJ_VAL(call_list));

    int arity = closure->function->arity;
    if(arity > 0) {
      write_list(vm, call_list, NIL_VAL); // accumulator
      if(arity > 1) {
        write_list(vm, call_list, NIL_VAL); // value
        if(arity > 2) {
          write_list(vm, call_list, NIL_VAL); // key
          if(arity > 3) {
            write_list(vm, call_list, METHOD_OBJECT); // list
          }
        }
      }
    }

    for(int i = start_index; i < dict->names.count; i++) {
      // only call map for non-empty values in a list.
      if(!IS_NIL(dict->names.values[i]) && !IS_EMPTY(dict->names.values[i])) {
        if(arity > 0) {
          call_list->items.values[0] = accumulator;
          if(arity > 1) {
            b_value value;
            table_get(&dict->items, dict->names.values[i], &value);
            call_list->items.values[1] = value;
            if(arity > 2) {
              call_list->items.values[2] = dict->names.values[i];
              if(arity > 4) {
                call_list->items.values[3] = METHOD_OBJECT;
              }
            }
          }
        }

        accumulator = call_closure(vm, closure, call_list);
      }
    }

    pop(vm); // pop the call list
    RETURN_VALUE(accumulator);
}

#undef ENFORCE_VALID_DICT_KEY