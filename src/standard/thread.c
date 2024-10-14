#include "module.h"
#include <pthread.h>
#include <sched.h>

#ifdef __linux__
#include <sys/syscall.h>
#elif defined(__FreeBSD__)
#include <sys/thr.h>
#elif defined(__NetBSD__)
#include <lwp.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include "bunistd.h"
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef SIGKILL
#define SIGKILL 9
#endif

typedef struct {
  pthread_t thread;
  b_vm *vm;
  b_obj_closure *closure;
  b_obj_list *args;
} b_thread_handle;

static uint64_t last_thread_vm_id = 0;

#define B_THREAD_PTR_NAME "<void *thread::thread>"

b_vm *copy_vm(b_vm *src, uint64_t id) {
  b_vm *vm = (b_vm *) malloc(sizeof(b_vm));
  if(!vm) {
    return NULL;
  }

  memset(vm, 0, sizeof(b_vm));

  vm->stack = ALLOCATE(b_value, COPIED_STACK_MIN);
  vm->stack_capacity = COPIED_STACK_MIN;

  // reset stack
  vm->stack_top = vm->stack;
  vm->error_top = vm->errors;
  vm->frame_count = 0;
  vm->open_up_values = NULL;

  // copies properties
  vm->compiler = src->compiler;
  vm->exception_class = src->exception_class;
  vm->root_file = src->root_file;
  vm->is_repl = src->is_repl;
  vm->show_warnings = src->show_warnings;
  vm->should_print_bytecode = src->should_print_bytecode;
  vm->should_exit_after_bytecode = src->should_exit_after_bytecode;
  vm->std_args = src->std_args;
  vm->std_args_count = src->std_args_count;

  // copied globals
  vm->modules = src->modules;
  vm->globals = src->globals;

  // every thread needs to maintain their own copy of the strings
  // without this, the threads will never terminate since the parent
  // vm always holds the root pointer to the strings
  // this will in turn lead to an infinite hang when creating
  // lots of threads in succession.
  init_table(&vm->strings);
  table_copy(vm, &src->strings, &vm->strings);

  // copied object methods
  vm->methods_string = src->methods_string;
  vm->methods_list = src->methods_list;
  vm->methods_dict = src->methods_dict;
  vm->methods_file = src->methods_file;
  vm->methods_bytes = src->methods_bytes;
  vm->methods_range = src->methods_range;

  // own properties
  vm->objects = NULL;
  vm->current_frame = NULL;
  vm->bytes_allocated = 0;
  vm->next_gc = DEFAULT_GC_START / 4; // default is quarter the original set value
  vm->mark_value = true;
  vm->gray_count = 0;
  vm->gray_capacity = 0;
  vm->gray_stack = NULL;

  vm->id = id;

  return vm;
}

static b_thread_handle *create_thread_handle(b_vm *vm, b_obj_closure *closure, b_obj_list *args) {
  if(last_thread_vm_id == UINT64_MAX) {
    // whatever makes us get here, due to resource constraint on devices,
    // the earlier threads definitely will no longer be in operation.
    last_thread_vm_id = 0;
  }

  b_thread_handle *handle = ALLOCATE(b_thread_handle, 1);
  if(handle != NULL) {
    handle->vm = copy_vm(vm, ++last_thread_vm_id);

    if(handle->vm == NULL) {
      FREE(b_thread_handle, handle);
      return NULL;
    }

    handle->closure = closure;
    handle->args = args;

    ((b_obj *)closure)->stale++;
    ((b_obj *)args)->stale++;
  }

  return handle;
}

static void free_thread_handle(b_thread_handle *thread) {
  if(thread != NULL && thread->vm != NULL) {
    free_vm(thread->vm);

    thread->vm = NULL;
    thread->closure = NULL;
    thread->args = NULL;

    ((b_obj *)thread)->stale--;
  }
}

static void b_free_thread_handle(void *data) {
  b_thread_handle *handle = (b_thread_handle *) data;
  free_thread_handle(handle);
  free(handle);
}

static void *b_thread_callback_function(void *data) {
  b_thread_handle *handle = (b_thread_handle *) data;
  if(handle == NULL || handle->vm == NULL) {
    pthread_exit(NULL);
  }

  for(int i = 0; i < handle->args->items.count; i++) {
    push(handle->vm, handle->args->items.values[i]);
  }

  if(run_closure_call(handle->vm, handle->closure, handle->args) == PTR_OK) {
    // do nothing for now...
  }

  ((b_obj *)handle->closure)->stale--;
  ((b_obj *)handle->args)->stale--;

  free_thread_handle(handle);
  pthread_exit(NULL);
}

DECLARE_MODULE_METHOD(thread__new) {
  ENFORCE_ARG_COUNT(new, 2);
  ENFORCE_ARG_TYPE(new, 0, IS_CLOSURE);
  ENFORCE_ARG_TYPE(new, 1, IS_LIST);

  b_thread_handle *thread = create_thread_handle(vm, AS_CLOSURE(args[0]), AS_LIST(args[1]));
  if(thread != NULL) {
    b_obj_ptr *ptr = new_closable_named_ptr(vm, thread, B_THREAD_PTR_NAME, b_free_thread_handle);
    ((b_obj *)ptr)->stale++;
    RETURN_OBJ(ptr);
  }

  RETURN_NIL;
}

DECLARE_MODULE_METHOD(thread__start) {
  ENFORCE_ARG_COUNT(start, 2);
  ENFORCE_ARG_TYPE(start, 0, IS_PTR);
  ENFORCE_ARG_TYPE(start, 1, IS_NUMBER);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, (size_t)AS_NUMBER(args[1]));

    if(pthread_create(&thread->thread, &attr, b_thread_callback_function, thread) == 0) {
      pthread_attr_destroy(&attr);
      RETURN_TRUE;
    }

    pthread_attr_destroy(&attr);
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(thread__dispose) {
  ENFORCE_ARG_COUNT(dispose, 1);
  ENFORCE_ARG_TYPE(dispose, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->vm != NULL) {
    if(pthread_kill(thread->thread, SIGKILL) == 0) {
      free_thread_handle(thread);
      RETURN_TRUE;
    }
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(thread__await) {
  ENFORCE_ARG_COUNT(await, 1);
  ENFORCE_ARG_TYPE(await, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->vm != NULL) {
    RETURN_BOOL(pthread_join(thread->thread, NULL) == 0);
  }

  RETURN_TRUE;
}

DECLARE_MODULE_METHOD(thread__detach) {
  ENFORCE_ARG_COUNT(detach, 1);
  ENFORCE_ARG_TYPE(detach, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->vm != NULL) {
    RETURN_BOOL(pthread_detach(thread->thread) == 0);
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(thread__set_name) {
  ENFORCE_ARG_COUNT(set_name, 2);
  ENFORCE_ARG_TYPE(set_name, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_name, 1, IS_STRING);
#ifdef __APPLE__
  RETURN_BOOL(pthread_setname_np(AS_C_STRING(args[1])) == 0);
#else
  b_thread_handle *thread = AS_PTR(args[0])->pointer;
# if defined(PTHREAD_MAX_NAMELEN_NP) && PTHREAD_MAX_NAMELEN_NP == 16
  RETURN_BOOL(pthread_setname_np(thread->thread, AS_C_STRING(args[1]), NULL) == 0);
# else
  RETURN_BOOL(pthread_setname_np(thread->thread, AS_C_STRING(args[1])) == 0);
# endif
#endif
}

DECLARE_MODULE_METHOD(thread__get_name) {
  ENFORCE_ARG_COUNT(get_name, 1);
  ENFORCE_ARG_TYPE(get_name, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  char buffer[255];
  if(pthread_getname_np(thread->thread, buffer, 255) == 0) {
    RETURN_STRING(buffer);
  }

  RETURN_VALUE(EMPTY_STRING_VAL);
}

uint64_t get_thread_id(void)
{
#if defined(__linux__)
  return syscall(SYS_gettid);
#elif defined(__FreeBSD__)
  /* thread id is up to INT_MAX */
    long tid;
    thr_self(&tid);
    return (uint64_t)tid;
#elif defined(__NetBSD__)
  return (uint64_t)_lwp_self();
#elif defined(__OpenBSD__)
  return (uint64_t)getthrid();
#elif defined(__APPLE__)
  uint64_t id;
  pthread_threadid_np(NULL, &id);
  return id;
#else
  return (uint64_t)getpid();
#endif
}

DECLARE_MODULE_METHOD(thread__get_id) {
  ENFORCE_ARG_COUNT(get_id, 0);
  RETURN_NUMBER(get_thread_id());
}

DECLARE_MODULE_METHOD(thread__yield) {
  ENFORCE_ARG_COUNT(yield, 0);
  RETURN_BOOL(sched_yield() == 0);
}

CREATE_MODULE_LOADER(thread) {
  static b_func_reg module_functions[] = {
      {"new", false, GET_MODULE_METHOD(thread__new)},
      {"start", false, GET_MODULE_METHOD(thread__start)},
      {"dispose", false, GET_MODULE_METHOD(thread__dispose)},
      {"await", false, GET_MODULE_METHOD(thread__await)},
      {"detach", false, GET_MODULE_METHOD(thread__detach)},
      {"yield", false, GET_MODULE_METHOD(thread__yield)},
      {"set_name", false, GET_MODULE_METHOD(thread__set_name)},
      {"get_name", false, GET_MODULE_METHOD(thread__get_name)},
      {"get_id", false, GET_MODULE_METHOD(thread__get_id)},
      {NULL,     false, NULL},
  };

  static b_module_reg module = {
      .name = "_thread",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}

#undef B_THREAD_PTR_NAME
