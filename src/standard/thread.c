#include "module.h"
#include "threads.h"

static uint64_t last_thread_vm_id = 0;

#define B_THREAD_PTR_NAME "<void *thread::thread>"

static b_thread_handle *create_thread_handle(b_vm *vm, b_obj_closure *closure, b_obj_list *args) {
  if(last_thread_vm_id == UINT64_MAX) {
    // whatever makes us get here, due to resource constraint on devices,
    // the earlier threads definitely will no longer be in operation.
    last_thread_vm_id = 0;
  }

  b_thread_handle *handle = ALLOCATE(b_thread_handle, 1);
  if(handle != NULL) {
    thrd_t *thread = ALLOCATE(thrd_t, 1);

    if(thread) {
      handle->thread = (void *)thread;
      handle->vm = copy_vm(vm, last_thread_vm_id++);
      handle->closure = closure;
      handle->args = args;
      handle->return_value[0] = EMPTY_VAL;

      ((b_obj *)closure)->stale = true;
      ((b_obj *)args)->stale = true;
    }
  }

  return handle;
}

static void b_free_thread_handle(void *data) {
  free_thread_handle((b_thread_handle *) data);
}

static int b_thread_callback_function(void *data) {
  b_thread_handle *handle = (b_thread_handle *) data;
  if(handle == NULL || handle->vm == NULL || handle->parent_vm == NULL) {
    thrd_exit(0);
    return 1;
  }

  for(int i = 0; i < handle->args->items.count; i++) {
    push(handle->vm, handle->args->items.values[i]);
  }

  if(run_closure_call(handle->vm, handle->closure, handle->args) == PTR_OK) {
    b_value result = handle->vm->stack_top[-1];
    handle->return_value[0] = result;
  }

  ((b_obj *)handle->closure)->stale = false;
  ((b_obj *)handle->args)->stale = false;

  return 0;
}

DECLARE_MODULE_METHOD(thread__new) {
  ENFORCE_ARG_COUNT(new, 2);
  ENFORCE_ARG_TYPE(new, 0, IS_CLOSURE);
  ENFORCE_ARG_TYPE(new, 1, IS_LIST);

  b_thread_handle *thread = create_thread_handle(vm, AS_CLOSURE(args[0]), AS_LIST(args[1]));
  if(thread) {
    push_thread(vm, thread);
    RETURN_CLOSABLE_NAMED_PTR(thread, B_THREAD_PTR_NAME, b_free_thread_handle);
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(thread__dispose) {
  ENFORCE_ARG_COUNT(dispose, 1);
  ENFORCE_ARG_TYPE(dispose, 0, IS_PTR);

  b_thread_handle *thread = AS_PTR(args[0])->pointer;
  free_thread_handle(thread);
  vm->threads_count--;
  RETURN;
}

DECLARE_MODULE_METHOD(thread__start) {
  ENFORCE_ARG_COUNT(start, 1);
  ENFORCE_ARG_TYPE(start, 0, IS_PTR);

  b_thread_handle *thread = AS_PTR(args[0])->pointer;
  RETURN_BOOL(thrd_create((thrd_t *)thread->thread, b_thread_callback_function, thread) == thrd_success);
}

DECLARE_MODULE_METHOD(thread__await) {
  ENFORCE_ARG_COUNT(await, 1);
  ENFORCE_ARG_TYPE(await, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;
  RETURN_BOOL(thrd_join(*((thrd_t *)thread->thread), 0) == thrd_success);
}

DECLARE_MODULE_METHOD(thread__detach) {
  ENFORCE_ARG_COUNT(detach, 1);
  ENFORCE_ARG_TYPE(detach, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;
  RETURN_BOOL(thrd_detach(*((thrd_t *)thread->thread)) == thrd_success);
}

CREATE_MODULE_LOADER(thread) {
  static b_func_reg module_functions[] = {
      {"new", false, GET_MODULE_METHOD(thread__new)},
      {"dispose", false, GET_MODULE_METHOD(thread__dispose)},
      {"start", false, GET_MODULE_METHOD(thread__start)},
      {"await", false, GET_MODULE_METHOD(thread__await)},
      {"detach", false, GET_MODULE_METHOD(thread__detach)},
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
