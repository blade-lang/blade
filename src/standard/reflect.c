#include "module.h"

extern bool call_value(b_vm *vm, b_value callee, int arg_count);

/**
 * hasprop(object: instance, name: string)
 *
 * returns true if object has the property name or false if not
 */
DECLARE_MODULE_METHOD(reflect__hasprop) {
  ENFORCE_ARG_COUNT(has_prop, 2);
  ENFORCE_ARG_TYPE(has_prop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(has_prop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value dummy;
  RETURN_BOOL(table_get(&instance->properties, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of the object matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
DECLARE_MODULE_METHOD(reflect__getprop) {
  ENFORCE_ARG_COUNT(get_prop, 2);
  ENFORCE_ARG_TYPE(get_prop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(get_prop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  if (table_get(&instance->properties, args[1], &value)) {
    RETURN_VALUE(value);
  }
  RETURN_NIL;
}

/**
 * setprop(object: instance, name: string, value: any)
 *
 * sets the named property of the object to value.
 *
 * if the property already exist, it overwrites it
 * @returns bool: true if a new property was set, false if a property was
 * updated
 */
DECLARE_MODULE_METHOD(reflect__setprop) {
  ENFORCE_ARG_COUNT(set_prop, 3);
  ENFORCE_ARG_TYPE(set_prop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(set_prop, 1, IS_STRING);
  ENFORCE_ARG_TYPE(set_prop, 2, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  RETURN_BOOL(table_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
DECLARE_MODULE_METHOD(reflect__delprop) {
  ENFORCE_ARG_COUNT(del_prop, 2);
  ENFORCE_ARG_TYPE(del_prop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(del_prop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  RETURN_BOOL(table_delete(&instance->properties, args[1]));
}

/**
 * hasmethod(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not
 */
DECLARE_MODULE_METHOD(reflect__hasmethod) {
  ENFORCE_ARG_COUNT(has_method, 2);
  ENFORCE_ARG_TYPE(has_method, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(has_method, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value dummy;
  RETURN_BOOL(table_get(&instance->klass->methods, args[1], &dummy));
}

/**
 * getmethod(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name
 */
DECLARE_MODULE_METHOD(reflect__getmethod) {
  ENFORCE_ARG_COUNT(get_method, 2);
  ENFORCE_ARG_TYPE(get_method, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(get_method, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  if (table_get(&instance->klass->methods, args[1], &value)) {
    RETURN_VALUE(value);
  }
  RETURN_NIL;
}

DECLARE_MODULE_METHOD(reflect__call_method) {
  ENFORCE_MIN_ARG(call_method, 3);
  ENFORCE_ARG_TYPE(call_method, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(call_method, 1, IS_STRING);
  ENFORCE_ARG_TYPE(call_method, 2, IS_LIST);

  b_value value;
  if (table_get(&AS_INSTANCE(args[0])->klass->methods, args[1], &value)) {
    b_obj_bound *bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(value)));

    b_obj_list *list = AS_LIST(args[2]);

    // remove the args list, the string name and the instance
    // then push the bound method
    pop_n(vm, 3);
    push(vm, OBJ_VAL(bound));

    // convert the list into function args
    for(int i = 0; i < list->items.count; i++) {
      push(vm, list->items.values[i]);
    }

    return call_value(vm, OBJ_VAL(bound), list->items.count);
  }

  RETURN;
}

DECLARE_MODULE_METHOD(reflect__bindmethod) {
  ENFORCE_ARG_COUNT(delist, 2);
  ENFORCE_ARG_TYPE(delist, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(delist, 1, IS_CLOSURE);

  b_obj_bound *bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(args[1])));
  RETURN_OBJ(bound);
}

DECLARE_MODULE_METHOD(reflect__getboundmethod) {
  ENFORCE_ARG_COUNT(get_method, 2);
  ENFORCE_ARG_TYPE(get_method, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(get_method, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  if (table_get(&instance->klass->methods, args[1], &value)) {
    b_obj_bound *bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(value)));
    RETURN_OBJ(bound);
  }
  RETURN_NIL;
}

DECLARE_MODULE_METHOD(reflect__gettype) {
  ENFORCE_ARG_COUNT(get_type, 1);
  ENFORCE_ARG_TYPE(get_type, 0, IS_INSTANCE);
  RETURN_OBJ(AS_INSTANCE(args[0])->klass->name);
}

DECLARE_MODULE_METHOD(reflect__isptr) {
  ENFORCE_ARG_COUNT(is_ptr, 1);
  RETURN_BOOL(IS_PTR(args[0]));
}

CREATE_MODULE_LOADER(reflect) {
  static b_func_reg module_functions[] = {
      {"hasprop",   true,  GET_MODULE_METHOD(reflect__hasprop)},
      {"getprop",   true,  GET_MODULE_METHOD(reflect__getprop)},
      {"setprop",   true,  GET_MODULE_METHOD(reflect__setprop)},
      {"delprop",   true,  GET_MODULE_METHOD(reflect__delprop)},
      {"hasmethod", true,  GET_MODULE_METHOD(reflect__hasmethod)},
      {"getmethod", true,  GET_MODULE_METHOD(reflect__getmethod)},
      {"getboundmethod", true,  GET_MODULE_METHOD(reflect__getboundmethod)},
      {"callmethod", true,  GET_MODULE_METHOD(reflect__call_method)},
      {"bindmethod", true,  GET_MODULE_METHOD(reflect__bindmethod)},
      {"gettype", true,  GET_MODULE_METHOD(reflect__gettype)},
      {"isptr", true,  GET_MODULE_METHOD(reflect__isptr)},
      {NULL,        false, NULL},
  };

  static b_module_reg module = {
      .name = "_reflect",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}