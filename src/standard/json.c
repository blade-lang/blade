#include "module.h"
#include <json.h>


b_value get_blade_value(b_vm *vm, json_value * data) {
  b_value value;
  switch (data->type) {
    case json_object: {
      b_obj_dict *dict = (b_obj_dict*)GC(new_dict(vm));
      value = OBJ_VAL(dict);
      for (int i = 0; i < data->u.object.length; i++) {
        b_obj_string *name = (b_obj_string *)GC(copy_string(vm, data->u.object.values[i].name, strlen(data->u.object.values[i].name)));
        b_value _value = get_blade_value(vm, data->u.object.values[i].value);
        dict_set_entry(vm, dict, OBJ_VAL(name), _value);
      }
      break;
    }
    case json_array: {
      b_obj_list *list = (b_obj_list*)GC(new_list(vm));
      value = OBJ_VAL(list);
      for (int i = 0; i < data->u.array.length; i++) {
        write_list(vm, list, get_blade_value(vm, data->u.array.values[i]));
      }
      break;
    }
    case json_integer: {
      value = NUMBER_VAL(data->u.integer);
      break;
    }
    case json_double: {
      value = NUMBER_VAL(data->u.dbl);
      break;
    }
    case json_string: {
      value = STRING_L_VAL(data->u.string.ptr, data->u.string.length);
      break;
    }
    case json_boolean: {
      value = BOOL_VAL((long)data->u.boolean);
      break;
    }
    default: {
      // covers json_null, json_none
      value = NIL_VAL;
      break;
    }
  }
  return value;
}


DECLARE_MODULE_METHOD(json__decode) {
  ENFORCE_ARG_RANGE(decode, 1, 2);
  ENFORCE_ARG_TYPE(decode, 0, IS_STRING);
  b_obj_string *data = AS_STRING(args[0]);

  json_settings settings;
  memset(&settings, 0, sizeof (json_settings));
  if(arg_count == 2 && !is_false(args[1])) {
    settings.settings = json_enable_comments;
  }
  char error[256];
  json_value * value = json_parse_ex(&settings, data->chars, data->length, error);
  if (value == 0) {
    RETURN_ERROR(error);
  }
  b_value converted = get_blade_value(vm, value);
  json_value_free(value);

  RETURN_VALUE(converted);
}


CREATE_MODULE_LOADER(json) {
  static b_func_reg module_functions[] = {
      {"_decode",   true,  GET_MODULE_METHOD(json__decode)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_json",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}