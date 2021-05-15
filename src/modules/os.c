#include "os.h"
#include "compat/unistd.h"

#ifdef _WIN32
#include "win32.h"
#else

#include <sys/utsname.h>

#endif

#include <ctype.h>
#include <stdio.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define sleep(s) Sleep((DWORD)s)
#endif // _WIN32

DECLARE_MODULE_METHOD(os_exec) {
  ENFORCE_ARG_COUNT(exec, 1);
  ENFORCE_ARG_TYPE(exec, 0, IS_STRING);
  b_obj_string *string = AS_STRING(args[0]);
  if (string->length == 0) {
    RETURN;
  }

  FILE *fd = popen(string->chars, "r");
  if (!fd)
    RETURN;

  char buffer[256];
  size_t n_read;
  size_t output_size = 256;
  int length = 0;
  char *output = malloc(output_size);

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
    length += (int)n_read;
  }

  if (length == 0)
    RETURN;

  output[length - 1] = '\0';

  pclose(fd);
  RETURN_L_STRING(output, length);
}

DECLARE_MODULE_METHOD(os_info) {
  ENFORCE_ARG_COUNT(info, 0);
  struct utsname os;
  if (uname(&os) != 0) {
    RETURN_ERROR("could not access os information");
  }

  b_obj_dict *dict = new_dict(vm);

  dict_add_entry(vm, dict, STRING_L_VAL("sysname", 7),
                 STRING_L_VAL(os.sysname, strlen(os.sysname)));
  dict_add_entry(vm, dict, STRING_L_VAL("nodename", 8),
                 STRING_L_VAL(os.nodename, strlen(os.nodename)));
  dict_add_entry(vm, dict, STRING_L_VAL("version", 7),
                 STRING_L_VAL(os.version, strlen(os.version)));
  dict_add_entry(vm, dict, STRING_L_VAL("release", 7),
                 STRING_L_VAL(os.release, strlen(os.release)));
  dict_add_entry(vm, dict, STRING_L_VAL("machine", 7),
                 STRING_L_VAL(os.machine, strlen(os.machine)));

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(os_sleep) {
  ENFORCE_ARG_COUNT(sleep, 1);
  ENFORCE_ARG_TYPE(sleep, 0, IS_NUMBER);
  sleep((int)AS_NUMBER(args[0]));
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

  RETURN_L_STRING(PLATFORM_NAME, (int)strlen(PLATFORM_NAME));

#undef PLATFORM_NAME
}

DECLARE_MODULE_METHOD(os_getenv) {
  ENFORCE_ARG_COUNT(getenv, 1);
  ENFORCE_ARG_TYPE(getenv, 0, IS_STRING);

  char *env = getenv(AS_C_STRING(args[0]));
  if (env != NULL)
    RETURN_STRING(env);
  else
    RETURN;
}

DECLARE_MODULE_METHOD(os_setenv) {
  ENFORCE_ARG_RANGE(setenv, 2, 3);
  ENFORCE_ARG_TYPE(setenv, 0, IS_STRING);
  ENFORCE_ARG_TYPE(setenv, 1, IS_STRING);

  int overwrite = 1;
  if (arg_count == 3) {
    ENFORCE_ARG_TYPE(setenv, 2, IS_BOOL);
    overwrite = AS_BOOL(args[2]) ? 1 : 0;
  }

#ifdef _WIN32
#define setenv(e, v, i) _putenv_s(e, v)
#endif

  if (setenv(AS_C_STRING(args[0]), AS_C_STRING(args[1]), overwrite) == 0)
    RETURN_TRUE;
  RETURN_FALSE;
}

CREATE_MODULE_LOADER(os) {
  static b_func_reg os_class_functions[] = {
      {"info", true, GET_MODULE_METHOD(os_info)},
      {"exec", true, GET_MODULE_METHOD(os_exec)},
      {"sleep", true, GET_MODULE_METHOD(os_sleep)},
      {"getenv", true, GET_MODULE_METHOD(os_getenv)},
      {"setenv", true, GET_MODULE_METHOD(os_setenv)},
      {NULL, false, NULL},
  };

  static b_field_reg os_class_fields[] = {
      {"platform", true, get_os_platform},
      {NULL, false, NULL},
  };

  static b_class_reg classes[] = {
      {"Os", os_class_fields, os_class_functions},
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL, classes};

  return module;
}