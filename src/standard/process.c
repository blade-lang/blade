#include "module.h"

#ifdef HAVE_SYSCONF
#include <unistd.h>
#endif
#ifdef HAVE_SYSCTLBYNAME
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(__MACOSX__) && (defined(__ppc__) || defined(__ppc64__))
#include <sys/sysctl.h>         /* For AltiVec check */
#elif defined(__OpenBSD__) && defined(__powerpc__)
#include <sys/param.h>
#include <sys/sysctl.h> /* For AltiVec check */
#include <machine/cpu.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#ifdef HAVE_SETJMP
#include <signal.h>
#include <setjmp.h>
#endif

b_value __process_cpu_count(b_vm *vm) {
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    return NUMBER_VAL((int)sysconf(_SC_NPROCESSORS_ONLN));
#endif
#ifdef HAVE_SYSCTLBYNAME
  int count = 0;
  size_t size = sizeof(count);
  sysctlbyname("hw.ncpu", &count, &size, NULL, 0);
  return NUMBER_VAL(count);
#endif
#ifdef __WIN32__
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return NUMBER_VAL(info.dwNumberOfProcessors);
#endif
#ifdef __OS2__
  int count = 0;
  DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS, &count, sizeof(count));
  return NUMBER_VAL(count);
#endif
  return NUMBER_VAL(1);
}

DECLARE_MODULE_METHOD(process_Process) {
  RETURN;
}

CREATE_MODULE_LOADER(process) {
  static b_func_reg os_module_functions[] = {
      {"Process",     false, GET_MODULE_METHOD(process_Process)},
      {NULL,     false, NULL},
  };

  static b_field_reg os_module_fields[] = {
      {"cpu_count", true, __process_cpu_count},
      {NULL,       false, NULL},
  };

  static b_module_reg module = {
      .name = "_process",
      .fields = os_module_fields,
      .functions = os_module_functions,
      .classes = NULL,
      .preloader= NULL,
      .unloader = NULL
  };

  return &module;
}
