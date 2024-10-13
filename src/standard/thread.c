#include "module.h"
#include <pthread.h>

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
    handle->vm = copy_vm(vm, ++last_thread_vm_id);

    if(handle->vm == NULL) {
      FREE(b_thread_handle, handle);
      return NULL;
    }

    handle->closure = closure;
    handle->args = args;

    ((b_obj *)closure)->stale = true;
    ((b_obj *)args)->stale = true;
  }

  return handle;
}


static void push_thread(b_vm *vm, b_thread_handle *thread) {
  if(vm->threads_capacity == vm->threads_count) {
    size_t capacity = GROW_CAPACITY(vm->threads_capacity);
    vm->threads = GROW_ARRAY(b_thread_handle *, vm->threads, vm->threads_capacity, capacity);
    vm->threads_capacity = capacity;

    vm->threads[vm->threads_count] = thread;
    thread->parent_thead_index = vm->threads_count;
    thread->parent_vm = vm;
  } else {
    for(int i = 0; i < vm->threads_capacity; i++) {
      if(vm->threads[i] == NULL) {
        vm->threads[i] = thread;
        thread->parent_thead_index = i;
        thread->parent_vm = vm;
        break;
      }
    }
  }

  vm->threads_count++;
}

static void free_thread_handle(b_thread_handle *thread) {
  if(thread != NULL && thread->parent_vm != NULL) {
    // make slot available for another thread

    thread->parent_vm->threads[thread->parent_thead_index] = NULL;
    thread->parent_vm->threads_count--;

    free_vm(thread->vm);

    thread->parent_vm = NULL;
    thread->vm = NULL;
    thread->closure = NULL;
    thread->args = NULL;

//    free(thread);
    thread = NULL;
  }
}

static void b_free_thread_handle(void *data) {
  b_thread_handle *handle = (b_thread_handle *) data;
  free_thread_handle(handle);
}

static void *b_thread_callback_function(void *data) {
  b_thread_handle *handle = (b_thread_handle *) data;
  if(handle == NULL || handle->vm == NULL || handle->parent_vm == NULL) {
    pthread_exit(NULL);
  }

  for(int i = 0; i < handle->args->items.count; i++) {
    push(handle->vm, handle->args->items.values[i]);
  }

  if(run_closure_call(handle->vm, handle->closure, handle->args) == PTR_OK) {
    // do nothing for now...
  }

  ((b_obj *)handle->closure)->stale = false;
  ((b_obj *)handle->args)->stale = false;

  free_thread_handle(handle);
  pthread_exit(NULL);
}

DECLARE_MODULE_METHOD(thread__run) {
  ENFORCE_ARG_COUNT(new, 2);
  ENFORCE_ARG_TYPE(new, 0, IS_CLOSURE);
  ENFORCE_ARG_TYPE(new, 1, IS_LIST);

  b_thread_handle *thread = create_thread_handle(vm, AS_CLOSURE(args[0]), AS_LIST(args[1]));
  if(thread != NULL) {
    push_thread(vm, thread);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64 * 1024);  // Reduce stack size to 64KB

    if(pthread_create(&thread->thread, &attr, b_thread_callback_function, thread) == 0) {
      pthread_attr_destroy(&attr);
      RETURN_CLOSABLE_NAMED_PTR(thread, B_THREAD_PTR_NAME, b_free_thread_handle);
    }

    pthread_attr_destroy(&attr);
  }

  RETURN_FALSE;
}

DECLARE_MODULE_METHOD(thread__dispose) {
  ENFORCE_ARG_COUNT(dispose, 1);
  ENFORCE_ARG_TYPE(dispose, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->thread && thread->vm != NULL) {
    pthread_kill(thread->thread, SIGABRT);
    free_thread_handle(thread);
  }

  RETURN;
}

DECLARE_MODULE_METHOD(thread__await) {
  ENFORCE_ARG_COUNT(await, 1);
  ENFORCE_ARG_TYPE(await, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->thread && thread->vm != NULL) {
    RETURN_BOOL(pthread_join(thread->thread, NULL) == 0);
  }

  RETURN_TRUE;
}

DECLARE_MODULE_METHOD(thread__detach) {
  ENFORCE_ARG_COUNT(detach, 1);
  ENFORCE_ARG_TYPE(detach, 0, IS_PTR);
  b_thread_handle *thread = AS_PTR(args[0])->pointer;

  if(thread != NULL && thread->thread && thread->vm != NULL) {
    RETURN_BOOL(pthread_detach(thread->thread) == 0);
  }

  RETURN_FALSE;
}

CREATE_MODULE_LOADER(thread) {
  static b_func_reg module_functions[] = {
      {"run", false, GET_MODULE_METHOD(thread__run)},
      {"dispose", false, GET_MODULE_METHOD(thread__dispose)},
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
