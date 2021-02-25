#include "module.h"
#include "pathinfo.h"
#include "value.h"

#include "modules/base64.h"
#include "modules/io.h"
#include "modules/math.h"
#include "modules/os.h"

typedef b_module_reg (*b_module_func)(b_vm *);

typedef struct {
  const char *name;
  b_module_func module_func;
} b_module_registry;

b_module_registry modules[] = {
    {"os", GET_MODULE_LOADER(os)},
    {"io", GET_MODULE_LOADER(io)},
    {"base64", GET_MODULE_LOADER(base64)},
    {"math", GET_MODULE_LOADER(math)},
    {NULL, NULL},
};

void bind_native_modules(b_vm *vm, b_obj_string *module_name,
                         const char *module_path) {

  if (is_core_library_file((char *)module_path, module_name->chars)) {
    for (int i = 0; modules[i].name != NULL; i++) {
      if (memcmp(modules[i].name, module_name->chars, module_name->length) ==
          0) {
        b_module_reg module = modules[i].module_func(vm);

        if (module.functions != NULL) {
          for (int j = 0; module.functions[j].name != NULL; j++) {
            b_func_reg func = module.functions[j];
            b_value func_name =
                OBJ_VAL(copy_string(vm, func.name, (int)strlen(func.name)));

            b_value func_real_value =
                OBJ_VAL(new_native(vm, func.function, func.name));

            table_set(vm, &vm->globals, func_name, func_real_value);
          }
        }

        if (module.klasses != NULL) {
          for (int j = 0; module.klasses[j].name != NULL; j++) {
            b_class_reg klass_reg = module.klasses[j];

            b_value class_key = OBJ_VAL(
                copy_string(vm, klass_reg.name, (int)strlen(klass_reg.name)));

            b_value class_value;
            if (table_get(&vm->globals, class_key, &class_value)) {
              b_obj_class *klass = AS_CLASS(class_value);

              if (klass_reg.functions != NULL) {
                for (int k = 0; klass_reg.functions[k].name != NULL; k++) {

                  b_func_reg func = klass_reg.functions[k];

                  b_value func_name = OBJ_VAL(
                      copy_string(vm, func.name, (int)strlen(func.name)));

                  b_value func_real_value =
                      OBJ_VAL(new_native(vm, func.function, func.name));

                  table_set(vm,
                            func.is_static ? &klass->static_methods
                                           : &klass->methods,
                            func_name, func_real_value);
                }
              }

              if (klass_reg.fields != NULL) {
                for (int j = 0; klass_reg.fields[j].name != NULL; j++) {
                  b_field_reg field = klass_reg.fields[j];
                  b_value field_name = OBJ_VAL(
                      copy_string(vm, field.name, (int)strlen(field.name)));

                  table_set(vm,
                            field.is_static ? &klass->static_fields
                                            : &klass->fields,
                            field_name, field.value);
                }
              }
            }
          }
        }
      }
    }
  }
}