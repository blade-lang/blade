#include "module.h"
#include "pathinfo.h"
#include "value.h"

#include "modules/os.h"

typedef b_module *(*b_module_func)(b_vm *);

typedef struct {
  const char *name;
  b_module_func module_func;
} b_module_registry;

b_module_registry modules[] = {
    {"os", GET_MODULE_LOADER(os)},
    {NULL, NULL},
};

void bind_native_modules(b_vm *vm, b_obj_string *module_name,
                         const char *module_path) {
  if (is_core_library_file((char *)module_path, module_name->chars)) {
    for (int i = 0; modules[i].name != NULL; i++) {
      if (memcmp(modules[i].name, module_name->chars, module_name->length) ==
          0) {
        b_module *module = modules[i].module_func(vm);
        b_func_reg *functions = module->functions;
        b_class_reg **klasses = module->klasses;

        if (functions != NULL) {
          for (int j = 0; j < module->function_count; j++) {
            b_func_reg func = functions[j];
            define_native_method(vm, &vm->globals, func.name, func.function);
          }
        }

        if (klasses != NULL) {
          for (int j = 0; j < module->class_count; j++) {
            b_class_reg *klass_reg = klasses[j];

            b_value class_key = OBJ_VAL(
                copy_string(vm, klass_reg->name, (int)strlen(klass_reg->name)));

            b_value class_value;
            if (table_get(&vm->globals, class_key, &class_value)) {
              b_obj_class *klass = AS_CLASS(class_value);

              for (int k = 0; k < klass_reg->function_count; k++) {

                b_native_fn func = klass_reg->functions[k];
                const char *name = klass_reg->names[k];
                // bind class methods here...
                define_native_method(vm, &klass->methods, name, func);
              }
            }
          }
        }
      }
    }
  }
}