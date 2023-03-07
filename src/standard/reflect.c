#include "module.h"
#include "compiler.h"

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
    int items_count = list->items.count;

    // remove the args list, the string name and the instance
    // then push the bound method
    pop_n(vm, 3);
    push(vm, OBJ_VAL(bound));

    // convert the list into function args
    for(int i = 0; i < items_count; i++) {
      push(vm, list->items.values[i]);
    }

    b_call_frame *frame = &vm->frames[vm->frame_count++];
    frame->closure = bound->method;
    frame->ip = bound->method->function->blob.code;

    frame->slots = vm->stack_top - items_count - 1;
    vm->current_frame = frame;
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

DECLARE_MODULE_METHOD(reflect__valueatdistance) {
  ENFORCE_ARG_COUNT(valueatdist, 1);
  RETURN_VALUE(vm->stack_top[(int)AS_NUMBER(args[0])]);
}

DECLARE_MODULE_METHOD(reflect__get_class_metadata) {
  ENFORCE_ARG_COUNT(get_class_metadata, 1);
  ENFORCE_ARG_TYPE(get_class_metadata, 0, IS_CLASS);
  b_obj_class *class = AS_CLASS(args[0]);

  b_obj_dict *result = (b_obj_dict *)GC(new_dict(vm));
  dict_set_entry(vm, result, GC_STRING("name"), OBJ_VAL(class->name));
  dict_set_entry(vm, result, GC_STRING("properties"), OBJ_VAL(table_get_keys(vm, &class->properties)));
  dict_set_entry(vm, result, GC_STRING("static_properties"), OBJ_VAL(table_get_keys(vm, &class->static_properties)));
  dict_set_entry(vm, result, GC_STRING("methods"), OBJ_VAL(table_get_keys(vm, &class->methods)));
  dict_set_entry(vm, result, GC_STRING("superclass"), class->superclass != NULL ? OBJ_VAL(class->superclass) : NIL_VAL);

  RETURN_OBJ(result);
}

DECLARE_MODULE_METHOD(reflect__get_function_metadata) {
  ENFORCE_ARG_COUNT(get_function_metadata, 1);
  ENFORCE_ARG_TYPE(get_function_metadata, 0, IS_CLOSURE);
  b_obj_closure *closure = AS_CLOSURE(args[0]);

  b_obj_dict *result = (b_obj_dict *)GC(new_dict(vm));
  dict_set_entry(vm, result, GC_STRING("name"), OBJ_VAL(closure->function->name));
  dict_set_entry(vm, result, GC_STRING("arity"), NUMBER_VAL(closure->function->arity));
  dict_set_entry(vm, result, GC_STRING("is_variadic"), NUMBER_VAL(closure->function->is_variadic));
  dict_set_entry(vm, result, GC_STRING("captured_vars"), NUMBER_VAL(closure->up_value_count));
  dict_set_entry(vm, result, GC_STRING("module"), STRING_VAL(closure->function->module->name));
  dict_set_entry(vm, result, GC_STRING("file"), STRING_VAL(closure->function->module->file));

  RETURN_OBJ(result);
}

DECLARE_MODULE_METHOD(reflect__get_module_metadata) {
  ENFORCE_ARG_COUNT(get_module_metadata, 1);
  ENFORCE_ARG_TYPE(get_module_metadata, 0, IS_MODULE);
  b_obj_module *module = AS_MODULE(args[0]);

  b_obj_dict *result = (b_obj_dict *)GC(new_dict(vm));
  dict_set_entry(vm, result, GC_STRING("name"), STRING_VAL(module->name));
  dict_set_entry(vm, result, GC_STRING("file"), STRING_VAL(module->file));
  dict_set_entry(vm, result, GC_STRING("has_preloader"), BOOL_VAL(module->preloader != NULL));
  dict_set_entry(vm, result, GC_STRING("has_unloader"), BOOL_VAL(module->unloader != NULL));
  dict_set_entry(vm, result, GC_STRING("definitions"), OBJ_VAL(table_get_keys(vm, &module->values)));

  RETURN_OBJ(result);
}

DECLARE_MODULE_METHOD(reflect__getclass) {
  ENFORCE_ARG_COUNT(get_type, 1);
  ENFORCE_ARG_TYPE(get_type, 0, IS_INSTANCE);
  RETURN_OBJ(AS_INSTANCE(args[0])->klass);
}

DECLARE_MODULE_METHOD(reflect__setglobal) {
  ENFORCE_ARG_RANGE(set_global, 1, 2);
  ENFORCE_ARG_TYPE(set_global, 0, IS_CLOSURE);
  b_obj_closure *cls = AS_CLOSURE(args[0]);
  b_obj_string *name;

  if(arg_count == 2 && !IS_NIL(args[2])) {
    ENFORCE_ARG_TYPE(set_global, 1, IS_STRING);
    name = AS_STRING(args[1]);
  } else {
    name = cls->function->name;
  }

  table_set(vm, &vm->globals, OBJ_VAL(name), OBJ_VAL(cls));
  RETURN;
}

DECLARE_MODULE_METHOD(reflect__runscript) {
  ENFORCE_ARG_COUNT(run_script, 1);
  ENFORCE_ARG_TYPE(run_script, 0, IS_STRING);
  char *source = AS_C_STRING(args[0]);

  b_blob  blob;
  init_blob(&blob);
  b_obj_func *fn = compile(vm, vm->current_frame->closure->function->module, source, &blob);
  if(fn != NULL) {
    push(vm, OBJ_VAL(fn));
    b_obj_closure *cls = new_closure(vm, fn);
    pop(vm);

    b_call_frame *frame = &vm->frames[vm->frame_count++];
    frame->closure = cls;
    frame->ip = fn->blob.code;

    frame->slots = vm->stack_top - 1;
    vm->current_frame = frame;
  }

  RETURN;
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
      {"getfunctionmetadata", true,  GET_MODULE_METHOD(reflect__get_function_metadata)},
      {"getclassmetadata", true,  GET_MODULE_METHOD(reflect__get_class_metadata)},
      {"getmodulemetadata", true,  GET_MODULE_METHOD(reflect__get_module_metadata)},
      {"getclass", true,  GET_MODULE_METHOD(reflect__getclass)},
      {"setglobal", true,  GET_MODULE_METHOD(reflect__setglobal)},
      {"runscript", true,  GET_MODULE_METHOD(reflect__runscript)},
      {"valueatdistance", true,  GET_MODULE_METHOD(reflect__valueatdistance)},
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