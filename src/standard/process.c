#include "module.h"

#ifdef HAVE_SYSCONF
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#ifdef HAVE_SYSCTLBYNAME
#include <sys/sysctl.h>
#endif

#ifdef _WIN32
#include "mman-win32/mman.h"
#else
#include <sys/mman.h>
#endif


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <sys/wait.h>
#else
#include "bunistd.h"
#endif /* HAVE_UNISTD_H */

#if defined(__OpenBSD__) && defined(__powerpc__)
#include <sys/param.h>
#include <machine/cpu.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#ifdef HAVE_SETJMP
#include <signal.h>
#endif

#include <errno.h>

// windows build stubs...
#ifdef _WIN32
#define WNOHANG 1
#define SIGKILL 1

static int fork() {
  errno = ENODEV;
  return -1;
}

static int waitpid(int i, void *j, int k) {
  return -1;
}

static int kill(int i, int j) {
  return -1;
}
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

typedef struct {
  pid_t pid;
} BProcess;

typedef struct {
  char *format;
  char *get_format;
  unsigned char *bytes;
  int format_length;
  int get_format_length;
  int length;
  bool locked;
} BProcessShared;

void b__free_shared_memory(void *data) {
  BProcessShared *shared = (BProcessShared *)data;
  munmap(shared->format, shared->format_length * sizeof(char));
  munmap(shared->get_format, shared->get_format_length * sizeof(char));
  munmap(shared->bytes, shared->length * sizeof(unsigned char));
  munmap(shared, sizeof(BProcessShared));
}

DECLARE_MODULE_METHOD(process_Process) {
  ENFORCE_ARG_RANGE(Process, 0, 1);
  BProcess *process = ALLOCATE(BProcess, 1);
  b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, process));
  ptr->name = "<*Process::Process>";
  process->pid = -1;
  RETURN_OBJ(ptr);
}

DECLARE_MODULE_METHOD(process_create) {
  ENFORCE_ARG_COUNT(create, 1);
  ENFORCE_ARG_TYPE(create, 0, IS_PTR);
  BProcess *process = (BProcess *) AS_PTR(args[0])->pointer;
  pid_t pid = fork();
  if(pid == -1) {
    RETURN_NUMBER(-1);
  } else if(!pid) {
    process->pid = getpid();
    RETURN_NUMBER(0);
  }
  RETURN_NUMBER(getpid());
}

DECLARE_MODULE_METHOD(process_is_alive) {
  ENFORCE_ARG_COUNT(create, 1);
  ENFORCE_ARG_TYPE(create, 0, IS_PTR);
  BProcess *process = (BProcess *) AS_PTR(args[0])->pointer;
  RETURN_BOOL(waitpid(process->pid, NULL, WNOHANG) == 0);
}

DECLARE_MODULE_METHOD(process_kill) {
  ENFORCE_ARG_COUNT(kill, 1);
  ENFORCE_ARG_TYPE(kill, 0, IS_PTR);
  BProcess *process = (BProcess *) AS_PTR(args[0])->pointer;
  RETURN_BOOL(kill(process->pid, SIGKILL) == 0);
}

DECLARE_MODULE_METHOD(process_wait) {
  ENFORCE_ARG_COUNT(create, 1);
  ENFORCE_ARG_TYPE(create, 0, IS_PTR);
  BProcess *process = (BProcess *) AS_PTR(args[0])->pointer;

  int status;
  waitpid(process->pid, &status, 0);

  pid_t p;
  do {
    p = waitpid(process->pid, &status, 0);
    if (p == -1) {
      if (errno == EINTR)
        continue;
      break;
    }
  } while (p != process->pid);

  if(p == process->pid) {
    RETURN_NUMBER(status);
  }
  RETURN_NUMBER(-1);
}

DECLARE_MODULE_METHOD(process_id) {
  ENFORCE_ARG_COUNT(create, 1);
  ENFORCE_ARG_TYPE(create, 0, IS_PTR);
  BProcess *process = (BProcess *) AS_PTR(args[0])->pointer;
  RETURN_NUMBER(process->pid);
}

DECLARE_MODULE_METHOD(process_new_shared) {
  ENFORCE_ARG_COUNT(new_shared, 0);
  BProcessShared *shared = mmap(NULL, sizeof(BProcessShared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared->bytes = mmap(NULL, sizeof(unsigned char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared->format = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared->get_format = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared->length = shared->get_format_length = shared->format_length = 0;
  b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, shared));
  ptr->name = "<*Process::SharedValue>";
  ptr->free_fn = b__free_shared_memory;
  RETURN_OBJ(ptr);
}

DECLARE_MODULE_METHOD(process_shared_write) {
  ENFORCE_ARG_COUNT(shared_write, 4);
  ENFORCE_ARG_TYPE(shared_write, 0, IS_PTR);
  ENFORCE_ARG_TYPE(shared_write, 1, IS_STRING);
  ENFORCE_ARG_TYPE(shared_write, 2, IS_STRING);
  ENFORCE_ARG_TYPE(shared_write, 3, IS_BYTES);

  BProcessShared *shared = (BProcessShared *)AS_PTR(args[0])->pointer;
  if(!shared->locked) {
    b_obj_string *format = AS_STRING(args[1]);
    b_obj_string *get_format = AS_STRING(args[2]);
    b_byte_arr bytes = AS_BYTES(args[3])->bytes;

    memcpy(shared->format, format->chars, format->length);
    shared->format_length = format->length;

    memcpy(shared->get_format, get_format->chars, get_format->length);
    shared->get_format_length = get_format->length;

    memcpy(shared->bytes, bytes.bytes, bytes.count);
    shared->length = bytes.count;

    // return length written
    RETURN_NUMBER(shared->length);
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(process_shared_read) {
  ENFORCE_ARG_COUNT(shared_read, 1);
  ENFORCE_ARG_TYPE(shared_read, 0, IS_PTR);
  BProcessShared *shared = (BProcessShared *)AS_PTR(args[0])->pointer;

  if(shared->length > 0 || shared->format_length > 0) {
    b_obj_bytes *bytes = (b_obj_bytes *) GC(copy_bytes(vm, shared->bytes, shared->length));

    // return [format, bytes]
    b_obj_list *list = (b_obj_list *)GC(new_list(vm));
    write_list(vm, list, GC_L_STRING(shared->get_format, shared->get_format_length));
    write_list(vm, list, OBJ_VAL(bytes));

    RETURN_OBJ(list);
  }
  RETURN_NIL;
}

DECLARE_MODULE_METHOD(process_shared_lock) {
  ENFORCE_ARG_COUNT(shared_lock, 1);
  ENFORCE_ARG_TYPE(shared_lock, 0, IS_PTR);
  BProcessShared *shared = (BProcessShared *)AS_PTR(args[0])->pointer;
  shared->locked = true;
  RETURN;
}

DECLARE_MODULE_METHOD(process_shared_unlock) {
  ENFORCE_ARG_COUNT(shared_unlock, 1);
  ENFORCE_ARG_TYPE(shared_unlock, 0, IS_PTR);
  BProcessShared *shared = (BProcessShared *)AS_PTR(args[0])->pointer;
  shared->locked = false;
  RETURN;
}

DECLARE_MODULE_METHOD(process_shared_islocked) {
  ENFORCE_ARG_COUNT(shared_islocked, 1);
  ENFORCE_ARG_TYPE(shared_islocked, 0, IS_PTR);
  BProcessShared *shared = (BProcessShared *)AS_PTR(args[0])->pointer;
  RETURN_BOOL(shared->locked);
}

CREATE_MODULE_LOADER(process) {
  static b_func_reg os_module_functions[] = {
      {"Process",     false, GET_MODULE_METHOD(process_Process)},
      {"create",     false, GET_MODULE_METHOD(process_create)},
      {"is_alive",     false, GET_MODULE_METHOD(process_is_alive)},
      {"wait",     false, GET_MODULE_METHOD(process_wait)},
      {"id",     false, GET_MODULE_METHOD(process_id)},
      {"kill",     false, GET_MODULE_METHOD(process_kill)},
      {"new_shared",     false, GET_MODULE_METHOD(process_new_shared)},
      {"shared_write",     false, GET_MODULE_METHOD(process_shared_write)},
      {"shared_read",     false, GET_MODULE_METHOD(process_shared_read)},
      {"shared_lock",     false, GET_MODULE_METHOD(process_shared_lock)},
      {"shared_unlock",     false, GET_MODULE_METHOD(process_shared_unlock)},
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
