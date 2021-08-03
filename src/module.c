#include "module.h"
#include "pathinfo.h"
#include "value.h"

#include "standard/standard.h"

b_module_init modules[] = {
    GET_MODULE_LOADER(os),         //
    GET_MODULE_LOADER(io),         //
    GET_MODULE_LOADER(base64), //
    GET_MODULE_LOADER(math),     //
    GET_MODULE_LOADER(date),     //
    GET_MODULE_LOADER(socket),     //
    GET_MODULE_LOADER(hash),     //
    NULL,
};

void bind_native_modules(b_vm *vm) {

  for (int i = 0; modules[i] != NULL; i++) {
    b_module_reg *module = modules[i](vm);

    if(module != NULL) {
      b_obj_module *the_module = new_module(vm, strdup(module->name), strdup("<__native__>"));
      the_module->preloader = module->preloader;
      the_module->unloader = module->unloader;

      if (module->fields != NULL) {
        for (int j = 0; module->fields[j].name != NULL; j++) {
          b_field_reg field = module->fields[j];
          b_value field_name =
              OBJ_VAL(copy_string(vm, field.name, (int) strlen(field.name)));

          table_set(vm, &the_module->values, field_name, field.field_value(vm));
        }
      }

      if (module->functions != NULL) {
        for (int j = 0; module->functions[j].name != NULL; j++) {
          b_func_reg func = module->functions[j];
          b_value func_name =
              OBJ_VAL(copy_string(vm, func.name, (int) strlen(func.name)));

          b_value func_real_value =
              OBJ_VAL(new_native(vm, func.function, func.name));

          table_set(vm, &the_module->values, func_name, func_real_value);
        }
      }

      if (module->classes != NULL) {
        for (int j = 0; module->classes[j].name != NULL; j++) {
          b_class_reg klass_reg = module->classes[j];

          b_obj_string *class_name = copy_string(vm, klass_reg.name, (int) strlen(klass_reg.name));

          b_obj_class *klass = new_class(vm, class_name);

          if (klass_reg.functions != NULL) {
            for (int k = 0; klass_reg.functions[k].name != NULL; k++) {

              b_func_reg func = klass_reg.functions[k];

              b_value func_name = OBJ_VAL(
                  copy_string(vm, func.name, (int) strlen(func.name)));

              b_obj_native *native = new_native(vm, func.function, func.name);

              if (func.is_static) {
                native->type = TYPE_STATIC;
              } else if (strlen(func.name) > 0 && func.name[0] == '_') {
                native->type = TYPE_PRIVATE;
              }

              table_set(vm, &klass->methods, func_name, OBJ_VAL(native));
            }
          }

          if (klass_reg.fields != NULL) {
            for (int k = 0; klass_reg.fields[k].name != NULL; k++) {
              b_field_reg field = klass_reg.fields[k];
              b_value field_name = OBJ_VAL(
                  copy_string(vm, field.name, (int) strlen(field.name)));

              table_set(vm,
                        field.is_static ? &klass->static_properties
                                        : &klass->properties,
                        field_name, field.field_value(vm));
            }
          }

          table_set(vm, &the_module->values, OBJ_VAL(class_name), OBJ_VAL(klass));
        }
      }

      add_native_module(vm, the_module);
    } else {
      // @TODO: Warn about module loading error...
    }
  }
}