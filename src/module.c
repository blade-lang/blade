#include "module.h"
#include "pathinfo.h"
#include "value.h"

#include "standard/standard.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#include "blade_dlfcn.h"
#endif

#if !defined(HAVE_DIRENT_H) || defined(_WIN32)
#include "dirent/dirent.h"
#else
#include <dirent.h>
#include <errno.h>
#endif /* HAVE_DIRENT_H */

#include <stdlib.h>
#include <sys/stat.h>

b_module_init modules[] = {
    GET_MODULE_LOADER(os),         //
    GET_MODULE_LOADER(io),         //
    GET_MODULE_LOADER(base64), //
    GET_MODULE_LOADER(math),     //
    GET_MODULE_LOADER(date),     //
    GET_MODULE_LOADER(socket),     //
    GET_MODULE_LOADER(hash),     //
    GET_MODULE_LOADER(reflect), //
    NULL,
};

bool load_module(b_vm *vm, b_module_init init_fn, char *import_name, char *source, void *handle) {
  b_module_reg *module = init_fn(vm);

  if(module != NULL) {
    b_obj_module *the_module = (b_obj_module*)GC(new_module(vm, strdup(module->name), source));
    the_module->preloader = module->preloader;
    the_module->unloader = module->unloader;

    if (module->fields != NULL) {
      for (int j = 0; module->fields[j].name != NULL; j++) {
        b_field_reg field = module->fields[j];
        b_value field_name = GC_STRING(field.name);

        table_set(vm, &the_module->values, field_name, field.field_value(vm));
      }
    }

    if (module->functions != NULL) {
      for (int j = 0; module->functions[j].name != NULL; j++) {
        b_func_reg func = module->functions[j];
        b_value func_name = GC_STRING(func.name);

        b_value func_real_value = OBJ_VAL(GC(new_native(vm, func.function, func.name)));

        table_set(vm, &the_module->values, func_name, func_real_value);
      }
    }

    if (module->classes != NULL) {
      for (int j = 0; module->classes[j].name != NULL; j++) {
        b_class_reg klass_reg = module->classes[j];

        b_obj_string *class_name = (b_obj_string *)GC(copy_string(vm, klass_reg.name, (int)strlen(klass_reg.name)));

        b_obj_class *klass = (b_obj_class *)GC(new_class(vm, class_name));

        if (klass_reg.functions != NULL) {
          for (int k = 0; klass_reg.functions[k].name != NULL; k++) {

            b_func_reg func = klass_reg.functions[k];

            b_value func_name = GC_STRING(func.name);

            b_obj_native *native = (b_obj_native*)GC(new_native(vm, func.function, func.name));

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
            b_value field_name = GC_STRING(field.name);

            table_set(vm,
                      field.is_static ? &klass->static_properties
                      : &klass->properties,
                      field_name, field.field_value(vm));
          }
        }

        table_set(vm, &the_module->values, OBJ_VAL(class_name), OBJ_VAL(klass));
      }
    }

    if(handle != NULL) {
      the_module->handle = handle;  // set handle for shared library modules
    }
    add_native_module(vm, the_module, the_module->name);

    CLEAR_GC();
    return true;
  } else {
    // @TODO: Warn about module loading error...
  }

  return false;
}

void add_native_module(b_vm *vm, b_obj_module *module, const char *as) {
  if(as != NULL) {
    module->name = strdup(as);
  }
  table_set(vm, &vm->modules, STRING_VAL(module->name), OBJ_VAL(module));
}

void bind_user_modules(b_vm *vm) {
  char *pkg_root = getenv(BLADE_PACKAGE_ROOT_ENV);
  if(pkg_root == NULL)
    pkg_root = merge_paths(get_exe_dir(), "dist");

  DIR *dir;
  if((dir = opendir(pkg_root)) != NULL) {
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {

      int ext_length = (int) strlen(LIBRARY_FILE_EXTENSION);

      // skip . and .. in path
      if (memcmp(ent->d_name, ".", (int)strlen(ent->d_name)) == 0
        || memcmp(ent->d_name, "..", (int)strlen(ent->d_name)) == 0
        || (int) strlen(ent->d_name) < ext_length + 1) {
        continue;
      }

      char *path = merge_paths(pkg_root, ent->d_name);
      if(!path) continue;

      int path_length = (int) strlen(path);

      struct stat sb;
      if(stat(path, &sb) == 0) {
        // it's not a directory
        if(S_ISDIR(sb.st_mode) < 1) {
          if(memcmp(path + (path_length - ext_length), LIBRARY_FILE_EXTENSION, ext_length) == 0) { // library file

            char *filename = get_real_file_name(path);

            int name_length = (int)strlen(filename) - ext_length;
            char *name = (char*) calloc(name_length + 1, sizeof(char));
            memcpy(name, filename, name_length);
            name[name_length] = '\0';

            char* error = load_user_module(vm, path, name);
            if(error != NULL) {
              // @TODO: handle appropriately
            }
          }
        }
      }
    }
    closedir(dir);
  }
}

void bind_native_modules(b_vm *vm) {
  for (int i = 0; modules[i] != NULL; i++) {
    load_module(vm, modules[i], NULL, strdup("<__native__>"), NULL);
  }
  bind_user_modules(vm);
}

char* load_user_module(b_vm *vm, const char *path, char *name) {
  int length = (int)strlen(name) + 20; // 20 == strlen("blade_module_loader_")
  char *fn_name = ALLOCATE(char, length + 1);

  if(fn_name == NULL) {
    return "failed to load module";
  }

  sprintf(fn_name, "blade_module_loader_%s", name);
  fn_name[length] = '\0'; // terminate the raw string

  void *handle;
  if((handle = dlopen(path, RTLD_LAZY)) == NULL) {
    return dlerror();
  }

  b_module_init fn = dlsym(handle, fn_name);
  if(fn == NULL) {
    return dlerror();
  }

  int path_length = (int)strlen(path);
  char *module_file = ALLOCATE(char, path_length + 1);
  memcpy(module_file, path, path_length);
  module_file[path_length] = '\0';

  if(!load_module(vm, fn, name, module_file, handle)) {
    FREE_ARRAY(char, fn_name, length + 1);
    FREE_ARRAY(char, module_file, path_length + 1);
    dlclose(handle);
    return "failed to call module loader";
  }

  return NULL;
}

void close_dl_module(void* handle) {
    dlclose(handle);
}
