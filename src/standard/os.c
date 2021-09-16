#include "os.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include "blade_unistd.h"
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#include <ctype.h>
#include <stdio.h>
#include <pathinfo.h>
#include <sys/stat.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose

#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifndef sleep
#define sleep(s) Sleep((DWORD)s)
#endif /* ifndef sleep */

/* Symbolic links aren't really a 'thing' on Windows, so just use plain-old
 * stat() instead of lstat(). */
#define lstat stat

#endif /* ifdef _WIN32 */

#if !defined(HAVE_DIRENT_H) || defined(_WIN32)
#include "dirent/dirent.h"
#else
#include <dirent.h>
#include <errno.h>
#endif /* HAVE_DIRENT_H */

#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#else
#include <errno.h>
#endif /* HAVE_SYS_ERRNO_H */

DECLARE_MODULE_METHOD(os_exec) {
  ENFORCE_ARG_COUNT(exec, 1);
  ENFORCE_ARG_TYPE(exec, 0, IS_STRING);
  b_obj_string *string = AS_STRING(args[0]);
  if (string->length == 0) {
    RETURN_NIL;
  }

  FILE *fd = popen(string->chars, "r");
  if (!fd) RETURN_NIL;

  char buffer[256];
  size_t n_read;
  size_t output_size = 256;
  int length = 0;
  char *output = ALLOCATE(char, output_size);

  if (output != NULL) {
    while ((n_read = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
      if (length + n_read >= output_size) {
        output_size *= 2;
        void *temp = realloc(output, output_size);
        if (temp == NULL) {
          RETURN_ERROR("device out of memory");
        } else {
          output = temp;
        }
      }
      if ((output + length) != NULL) {
        strncat(output + length, buffer, n_read);
      }
      length += (int) n_read;
    }

    if (length == 0) {
      pclose(fd);
      RETURN_NIL;
    }

    output[length - 1] = '\0';

    pclose(fd);
    RETURN_T_STRING(output, length);
  }

  pclose(fd);
  RETURN_STRING("");
}

DECLARE_MODULE_METHOD(os_info) {
  ENFORCE_ARG_COUNT(info, 0);

#ifdef HAVE_SYS_UTSNAME_H
  struct utsname os;
  if (uname(&os) != 0) {
    RETURN_ERROR("could not access os information");
  }

  b_obj_dict *dict = new_dict(vm);
  push(vm, OBJ_VAL(dict));

  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "sysname", 7)),
                 OBJ_VAL(copy_string(vm, os.sysname, strlen(os.sysname))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "nodename", 8)),
                 OBJ_VAL(copy_string(vm, os.nodename, strlen(os.nodename))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "version", 7)),
                 OBJ_VAL(copy_string(vm, os.version, strlen(os.version))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "release", 7)),
                 OBJ_VAL(copy_string(vm, os.release, strlen(os.release))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "machine", 7)),
                 OBJ_VAL(copy_string(vm, os.machine, strlen(os.machine))));

  pop(vm);
  RETURN_OBJ(dict);
#else
  RETURN_ERROR("not available: OS does not have uname()")
#endif /* HAVE_SYS_UTSNAME_H */
}

DECLARE_MODULE_METHOD(os_sleep) {
  ENFORCE_ARG_COUNT(sleep, 1);
  ENFORCE_ARG_TYPE(sleep, 0, IS_NUMBER);
  sleep((int) AS_NUMBER(args[0]));
  RETURN;
}

b_value get_os_platform(b_vm *vm) {

#if defined(_WIN32)
#define PLATFORM_NAME "windows" // Windows
#elif defined(_WIN64)
#define PLATFORM_NAME "windows" // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define PLATFORM_NAME "windows" // Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__ANDROID__)
#define PLATFORM_NAME                                                          \
  "android" // Android (implies Linux, so it must come first)
#elif defined(__linux__)
#define PLATFORM_NAME                                                          \
  "linux" // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
#include <sys/param.h>
#if defined(BSD)
#define PLATFORM_NAME "bsd" // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#endif
#elif defined(__hpux)
#define PLATFORM_NAME "hp-ux" // HP-UX
#elif defined(_AIX)
#define PLATFORM_NAME "aix"                   // IBM AIX
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)

#include <TargetConditionals.h>

#if TARGET_IPHONE_SIMULATOR == 1
#define PLATFORM_NAME "ios" // Apple iOS
#elif TARGET_OS_IPHONE == 1
#define PLATFORM_NAME "ios" // Apple iOS
#elif TARGET_OS_MAC == 1
#define PLATFORM_NAME "osx" // Apple OSX
#endif
#elif defined(__sun) && defined(__SVR4)
#define PLATFORM_NAME "solaris" // Oracle Solaris, Open Indiana
#elif defined(__OS400__)
#define PLATFORM_NAME "ibm" // IBM OS/400
#elif defined(AMIGA) || defined(__MORPHOS__)
#define PLATFORM_NAME "amiga"
#else
#define PLATFORM_NAME "unknown"
#endif

  return OBJ_VAL(copy_string(vm, PLATFORM_NAME, (int) strlen(PLATFORM_NAME)));

#undef PLATFORM_NAME
}

b_value get_blade_os_args(b_vm *vm) {
  b_obj_list *list = (b_obj_list*)GC(new_list(vm));
  if(vm->std_args != NULL) {
    for(int i = 0; i < vm->std_args_count; i++) {
      write_list(vm, list, STRING_VAL(vm->std_args[i]));
    }
  }
  CLEAR_GC();
  return OBJ_VAL(list);
}

b_value get_blade_os_path_separator(b_vm *vm) {
  return STRING_L_VAL(BLADE_PATH_SEPARATOR, 1);
}

DECLARE_MODULE_METHOD(os_getenv) {
  ENFORCE_ARG_COUNT(get_env, 1);
  ENFORCE_ARG_TYPE(get_env, 0, IS_STRING);

  char *env = getenv(AS_C_STRING(args[0]));
  if (env != NULL) {
    RETURN_STRING(env);
  } else {
    RETURN_NIL;
  }
}

DECLARE_MODULE_METHOD(os_setenv) {
  ENFORCE_ARG_RANGE(set_env, 2, 3);
  ENFORCE_ARG_TYPE(set_env, 0, IS_STRING);
  ENFORCE_ARG_TYPE(set_env, 1, IS_STRING);

  int overwrite = 1;
  if (arg_count == 3) {
    ENFORCE_ARG_TYPE(setenv, 2, IS_BOOL);
    overwrite = AS_BOOL(args[2]) ? 1 : 0;
  }

#ifdef _WIN32
#define setenv(e, v, i) _putenv_s(e, v)
#endif

  if (setenv(AS_C_STRING(args[0]), AS_C_STRING(args[1]), overwrite) == 0) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(os__createdir) {
  ENFORCE_ARG_COUNT(create_dir, 3);
  ENFORCE_ARG_TYPE(create_dir, 0, IS_STRING);
  ENFORCE_ARG_TYPE(create_dir, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(create_dir, 2, IS_BOOL);

  b_obj_string *path = AS_STRING(args[0]);
  int mode = AS_NUMBER(args[1]);
  bool is_recursive = AS_BOOL(args[2]);

  char sep = BLADE_PATH_SEPARATOR[0];
  bool exists = false;

  if(is_recursive) {
    for (char* p = strchr(path->chars + 1, sep); p; p = strchr(p + 1, sep)) {
      *p = '\0';
#ifdef _WIN32
      if (!CreateDirectory(path->chars, NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
          *p = sep;
          RETURN_ERROR(strerror(GetLastError()));
#else
      if (mkdir(path->chars, mode) == -1) {
        if (errno != EEXIST) {
          *p = sep;
          RETURN_ERROR(strerror(errno));
#endif /* _WIN32 */
        } else {
          exists = true;
        }
      } else {
        exists = false;
      }
//      chmod(path->chars, (mode_t) mode);
      *p = sep;
    }

  } else {

#ifdef _WIN32
    if (!CreateDirectory(path->chars, NULL)) {
      if (GetLastError() != ERROR_ALREADY_EXISTS) {
        RETURN_ERROR(strerror(GetLastError()));
#else
    if (mkdir(path->chars, mode) == -1) {
      if (errno != EEXIST) {
        RETURN_ERROR(strerror(errno));
#endif /* _WIN32 */
      } else {
        exists = true;
      }
    }
//    chmod(path->chars, (mode_t) mode);

  }

  RETURN_BOOL(!exists);
}

DECLARE_MODULE_METHOD(os__readdir) {
  ENFORCE_ARG_COUNT(read_dir, 1);
  ENFORCE_ARG_TYPE(read_dir, 0, IS_STRING);
  b_obj_string *path = AS_STRING(args[0]);

  DIR *dir;
  if((dir = opendir(path->chars)) != NULL) {
    b_obj_list *list = (b_obj_list *)GC(new_list(vm));
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
      write_list(vm, list, STRING_VAL(ent->d_name));
    }
    closedir(dir);
    RETURN_OBJ(list);
  }
  RETURN_ERROR(strerror(errno));
}

static int remove_directory(char *path, int path_length, bool recursive) {
  DIR *dir;
  if((dir = opendir(path)) != NULL) {
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {

      // skip . and .. in path
      if (memcmp(ent->d_name, ".", (int)strlen(ent->d_name)) == 0
      || memcmp(ent->d_name, "..", (int)strlen(ent->d_name)) == 0) {
        continue;
      }

      int path_string_length = path_length + (int)strlen(ent->d_name) + 2;
      char *path_string = (char*) calloc(path_string_length, sizeof(char));
      if(path_string == NULL) return -1;

      snprintf(path_string, path_string_length, "%s" BLADE_PATH_SEPARATOR "%s", path, ent->d_name);

      struct stat sb;
      if(stat(path_string, &sb) == 0) {
        if(S_ISDIR(sb.st_mode) > 0 && recursive) {
          // recurse
          if(remove_directory(path_string, path_string_length, recursive) == -1) {
            free(path_string);
            return -1;
          }
        } else if(unlink(path_string) == -1) {
          free(path_string);
          return -1;
        } else {
          free(path_string);
        }
      } else {
        free(path_string);
        return -1;
      }
    }
    closedir(dir);
    return rmdir(path);
  }
  return -1;
}

DECLARE_MODULE_METHOD(os__removedir){
  ENFORCE_ARG_COUNT(remove_dir, 2);
  ENFORCE_ARG_TYPE(remove_dir, 0, IS_STRING);
  ENFORCE_ARG_TYPE(remove_dir, 1, IS_BOOL);

  b_obj_string *path = AS_STRING(args[0]);
  bool recursive = AS_BOOL(args[1]);
  if(remove_directory(path->chars, path->length, recursive) >= 0) {
    RETURN_TRUE;
  }
  RETURN_ERROR(strerror(errno));
}

DECLARE_MODULE_METHOD(os__chmod) {
  ENFORCE_ARG_COUNT(chmod, 2);
  ENFORCE_ARG_TYPE(chmod, 0, IS_STRING);
  ENFORCE_ARG_TYPE(chmod, 1, IS_NUMBER);

  b_obj_string *path = AS_STRING(args[0]);
  int mode = AS_NUMBER(args[1]);
  if(chmod(path->chars, mode) != 0) {
    RETURN_ERROR(strerror(errno));
  }
  RETURN_TRUE;
}

DECLARE_MODULE_METHOD(os__is_dir) {
  ENFORCE_ARG_COUNT(is_dir, 1);
  ENFORCE_ARG_TYPE(is_dir, 0, IS_STRING);
  b_obj_string *path = AS_STRING(args[0]);
  struct stat sb;
  if(stat(path->chars, &sb) == 0) {
    RETURN_BOOL(S_ISDIR(sb.st_mode) > 0);
  }
  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(os__exit) {
  ENFORCE_ARG_COUNT(exit, 1);
  ENFORCE_ARG_TYPE(exit, 0, IS_NUMBER);
  exit((int)AS_NUMBER(args[0]));
  RETURN;
}

DECLARE_MODULE_METHOD(os__cwd) {
  ENFORCE_ARG_COUNT(cwd, 0);
  char *cwd = getcwd(NULL, 0);
  if(cwd != NULL) {
    RETURN_TT_STRING(cwd);
  }
  RETURN_L_STRING("",1);
}

DECLARE_MODULE_METHOD(os__realpath) {
  ENFORCE_ARG_COUNT(_realpath, 1);
  ENFORCE_ARG_TYPE(_realpath, 0, IS_STRING);
  char *path = realpath(AS_C_STRING(args[0]), NULL);
  RETURN_T_STRING(path,(int) strlen(path));
}

DECLARE_MODULE_METHOD(os__chdir) {
  ENFORCE_ARG_COUNT(chdir, 1);
  ENFORCE_ARG_TYPE(chdir, 0, IS_STRING);
  RETURN_BOOL(chdir(AS_STRING(args[0])->chars) == 0);
}

DECLARE_MODULE_METHOD(os__exists) {
  ENFORCE_ARG_COUNT(exists, 1);
  ENFORCE_ARG_TYPE(exists, 0, IS_STRING);
  struct stat sb;
  if(stat(AS_STRING(args[0])->chars, &sb) == 0 && sb.st_mode & S_IFDIR) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

/** DIR TYPES BEGIN */

b_value __os_dir_DT_UNKNOWN(b_vm *vm){
  return NUMBER_VAL(DT_UNKNOWN);
}

b_value __os_dir_DT_REG(b_vm *vm){
  return NUMBER_VAL(DT_REG);
}

b_value __os_dir_DT_DIR(b_vm *vm){
  return NUMBER_VAL(DT_DIR);
}

b_value __os_dir_DT_FIFO(b_vm *vm){
  return NUMBER_VAL(DT_FIFO);
}

b_value __os_dir_DT_SOCK(b_vm *vm){
  return NUMBER_VAL(DT_SOCK);
}

b_value __os_dir_DT_CHR(b_vm *vm){
  return NUMBER_VAL(DT_CHR);
}

b_value __os_dir_DT_BLK(b_vm *vm) {
  return NUMBER_VAL(DT_BLK);
}

b_value __os_dir_DT_LNK(b_vm *vm) {
  return NUMBER_VAL(DT_LNK);
}

b_value __os_dir_DT_WHT(b_vm *vm) {
#ifdef DT_WHT
  return NUMBER_VAL(DT_WHT);
#else
  return NUMBER_VAL(-1);
#endif
}

/** DIR TYPES ENDS */

CREATE_MODULE_LOADER(os) {
  static b_func_reg os_module_functions[] = {
      {"_info",   true,  GET_MODULE_METHOD(os_info)},
      {"_exec",   true,  GET_MODULE_METHOD(os_exec)},
      {"_sleep",  true,  GET_MODULE_METHOD(os_sleep)},
      {"_getenv", true,  GET_MODULE_METHOD(os_getenv)},
      {"_setenv", true,  GET_MODULE_METHOD(os_setenv)},
      {"_createdir", true,  GET_MODULE_METHOD(os__createdir)},
      {"_readdir", true,  GET_MODULE_METHOD(os__readdir)},
      {"_chmod", true,  GET_MODULE_METHOD(os__chmod)},
      {"_isdir", true,  GET_MODULE_METHOD(os__is_dir)},
      {"_exit", true,  GET_MODULE_METHOD(os__exit)},
      {"_cwd", true,  GET_MODULE_METHOD(os__cwd)},
      {"_removedir", true,  GET_MODULE_METHOD(os__removedir)},
      {"_chdir", true,  GET_MODULE_METHOD(os__chdir)},
      {"_exists", true,  GET_MODULE_METHOD(os__exists)},
      {"_realpath", true,  GET_MODULE_METHOD(os__realpath)},
      {NULL,     false, NULL},
  };

  static b_field_reg os_module_fields[] = {
      {"_platform", true, get_os_platform},
      {"_args", true, get_blade_os_args},
      {"_path_separator", true, get_blade_os_path_separator},
      {"_DT_UNKNOWN", true, __os_dir_DT_UNKNOWN},
      {"_DT_BLK", true, __os_dir_DT_BLK},
      {"_DT_CHR", true, __os_dir_DT_CHR},
      {"_DT_DIR", true, __os_dir_DT_DIR},
      {"_DT_FIFO", true, __os_dir_DT_FIFO},
      {"_DT_LNK", true, __os_dir_DT_LNK},
      {"_DT_REG", true, __os_dir_DT_REG},
      {"_DT_SOCK", true, __os_dir_DT_SOCK},
      {"_DT_WHT", true, __os_dir_DT_WHT},
      {NULL,       false, NULL},
  };

  static b_module_reg module = {
      .name = "_os",
      .fields = os_module_fields,
      .functions = os_module_functions,
      .classes = NULL,
      .preloader= NULL,
      .unloader = NULL
  };

  return &module;
}
