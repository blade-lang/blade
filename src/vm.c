#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "config.h"
#include "memory.h"
#include "native.h"
#include "object.h"
#include "vm.h"

#if DEBUG_MODE == 1
#include "debug.h"
#endif

#define runtime_error(...)                                                     \
  _runtime_error(vm, ##__VA_ARGS__);                                           \
  return PTR_RUNTIME_ERR

static void reset_stack(b_vm *vm) {
  vm->stack_top = vm->stack;
  vm->frame_count = 0;
  vm->open_upvalues = NULL;
}

void _runtime_error(b_vm *vm, const char *format, ...) {

  b_call_frame *frame = &vm->frames[vm->frame_count - 1];

  size_t instruction = frame->ip - frame->closure->function->blob.code - 1;
  int line = frame->closure->function->blob.lines[instruction];

  fprintf(stderr, "RuntimeError:\n");
  fprintf(stderr, "    File: <script>, Line: %d\n    Message: ", line);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  fprintf(stderr, "StackTrace:\n");
  for (int i = vm->frame_count - 1; i >= 0; i--) {
    b_call_frame *frame = &vm->frames[i];
    b_obj_func *function = frame->closure->function;

    // -1 because the IP is sitting on the next instruction to be executed
    size_t instruction = frame->ip - frame->closure->function->blob.code - 1;

    fprintf(stderr, "    File: <script>, Line: %d, In: ",
            function->blob.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "<script>\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_stack(vm);
}

void push(b_vm *vm, b_value value) {
  *vm->stack_top = value;
  vm->stack_top++;
}

b_value pop(b_vm *vm) {
  vm->stack_top--;
  return *vm->stack_top;
}

b_value popn(b_vm *vm, int n) {
  vm->stack_top -= n;
  return *vm->stack_top;
}

static b_value peek(b_vm *vm, int distance) {
  return vm->stack_top[-1 - distance];
}

static void define_native(b_vm *vm, const char *name, b_native_fn function) {
  push(vm, OBJ_VAL(copy_string(vm, name, (int)strlen(name))));
  push(vm, OBJ_VAL(new_native(vm, function, name)));
  table_set(vm, &vm->globals, vm->stack[0], vm->stack[1]);
  popn(vm, 2);
}

void init_vm(b_vm *vm) {
  reset_stack(vm);
  vm->compiler = NULL;
  vm->objects = NULL;
  vm->bytes_allocated = 0;
  vm->next_gc = 1024 * 1024; // 1mb

  vm->gray_count = 0;
  vm->gray_capacity = 0;
  vm->gray_stack = NULL;

  init_table(&vm->strings);
  init_table(&vm->globals);

  DEFINE_NATIVE(time);
  DEFINE_NATIVE(microtime);
  DEFINE_NATIVE(id);
}

void free_vm(b_vm *vm) {
  free_objects(vm);
  free_table(vm, &vm->strings);
  free_table(vm, &vm->globals);
}

static bool call(b_vm *vm, b_obj_closure *closure, int arg_count) {
  if (arg_count != closure->function->arity) {
    _runtime_error(vm, "expected %d arguments but got %d",
                   closure->function->arity, arg_count);
    return false;
  }

  if (vm->frame_count == FRAMES_MAX) {
    _runtime_error(vm, "stack overflow");
    return false;
  }

  b_call_frame *frame = &vm->frames[vm->frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->blob.code;

  frame->slots = vm->stack_top - arg_count - 1;
  return true;
}

static bool call_value(b_vm *vm, b_value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_CLOSURE: {
      return call(vm, AS_CLOSURE(callee), arg_count);
    }

    case OBJ_NATIVE: {
      b_native_fn native = AS_NATIVE(callee)->function;
      b_value result = native(vm, arg_count, vm->stack_top - arg_count);
      if (IS_EMPTY(result)) {
        return false;
      } else if (!IS_UNDEFINED(result)) {
        vm->stack_top -= arg_count + 1;
        push(vm, result);
      }
      return true;
    }

    default: // non callable
      break;
    }
  }
  _runtime_error(vm, "only functions and classes can be called");
  return false;
}

static b_obj_upvalue *capture_upvalue(b_vm *vm, b_value *local) {
  b_obj_upvalue *prev_upvalue = NULL;
  b_obj_upvalue *upvalue = vm->open_upvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local)
    return upvalue;

  b_obj_upvalue *created_upvalue = new_upvalue(vm, local);
  created_upvalue->next = upvalue;

  if (prev_upvalue == NULL) {
    vm->open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }

  return created_upvalue;
}

static void close_upvalues(b_vm *vm, b_value *last) {
  while (vm->open_upvalues != NULL && vm->open_upvalues->location >= last) {
    b_obj_upvalue *upvalue = vm->open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm->open_upvalues = upvalue->next;
  }
}

static bool is_falsey(b_value value) {
  if (IS_BOOL(value))
    return IS_BOOL(value) && !AS_BOOL(value);
  if (IS_NIL(value))
    return true;

  // -1 is the number equivalent of false in Birdy
  if (IS_NUMBER(value))
    return AS_NUMBER(value) < 0;

  // Non-empty strings are true, empty strings are false.
  if (IS_STRING(value))
    return strlen(AS_STRING(value)->chars) < 1;

  /* // Non-empty lists are true, empty lists are false.
  if (IS_LIST(value))
    return AS_LIST(value)->values.count == 0;

  // Non-empty dicts are true, empty dicts are false.
  if (IS_DICT(value))
    return AS_DICT(value)->names.count == 0;

  // All classes are true
  // All functions are in themselves true if you do not account
  // for what they return.
  if (IS_CLASS(value) || IS_CLOSURE(value) || IS_FUNCTION(value) ||
      IS_BOUND_METHOD(value))
    return false; */

  return false;
}

static bool concatenate(b_vm *vm) {
  b_value _b = peek(vm, 0);
  b_value _a = peek(vm, 1);

  if (IS_NIL(_a)) {
    pop(vm);
    pop(vm);
    push(vm, _b);
  } else if (IS_NIL(_b)) {
    pop(vm);
  } else if (IS_NUMBER(_a)) {
    double a = AS_NUMBER(_a);

    char num_str[200];
    sprintf(num_str, NUMBER_FORMAT, a);
    int num_length = strlen(num_str);

    b_obj_string *b = AS_STRING(_b);

    int length = num_length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, num_str, num_length);
    memcpy(chars + num_length, b->chars, b->length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
  } else if (IS_NUMBER(_b)) {
    b_obj_string *a = AS_STRING(_a);
    double b = AS_NUMBER(_b);

    char num_str[200];
    sprintf(num_str, NUMBER_FORMAT, b);
    int num_length = strlen(num_str);

    int length = num_length + a->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, num_str, num_length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
  } else if (IS_STRING(_a) && IS_STRING(_b)) {
    b_obj_string *b = AS_STRING(_b);
    b_obj_string *a = AS_STRING(_a);

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    b_obj_string *result = take_string(vm, chars, length);

    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
  } else {
    return false;
  }

  return true;
}

static int floor_div(double a, double b) {
  int d = (int)a / (int)b;
  return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

b_ptr_result run(b_vm *vm) {

  b_call_frame *frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT()                                                        \
  (frame->closure->function->blob.constants.values[READ_SHORT()])

#define READ_STRING() (AS_STRING(READ_CONSTANT()))

#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      _runtime_error(vm, "unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(a op b));                                                    \
  } while (false)

#define BINARY_MOD_OP(type, op)                                                \
  do {                                                                         \
    if ((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) ||                  \
        (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) {                  \
      _runtime_error(vm, "unsupported operand %s for %s and %s", #op,          \
                     value_type(peek(vm, 0)), value_type(peek(vm, 1)));        \
    }                                                                          \
    b_value _b = pop(vm);                                                      \
    double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);            \
    b_value _a = pop(vm);                                                      \
    double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);            \
    push(vm, type(op(a, b)));                                                  \
  } while (false)

  for (;;) {

#if DEBUG_MODE == 1
#if DEBUG_TRACE_EXECUTION == 1
    printf("          ");
    for (b_value *slot = vm->stack; slot < vm->stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(
        frame->closure->function->blob,
        (int)(frame->ip - frame->closure->function->blob.code));
#endif
#endif

    uint8_t instruction;

    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      b_value constant = READ_CONSTANT();
      push(vm, constant);
      break;
    }

    case OP_ADD: {
      if (IS_STRING(peek(vm, 0)) || IS_STRING(peek(vm, 1))) {
        if (!concatenate(vm)) {
          runtime_error("unsupported operand + for %s and %s",
                        value_type(peek(vm, 0)), value_type(peek(vm, 1)));
        }
      } else {
        BINARY_OP(NUMBER_VAL, +);
      }
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_REMINDER: {
      BINARY_MOD_OP(NUMBER_VAL, fmod);
      break;
    }
    case OP_POW: {
      BINARY_MOD_OP(NUMBER_VAL, pow);
      break;
    }
    case OP_FDIVIDE: {
      BINARY_MOD_OP(NUMBER_VAL, floor_div);
      break;
    }
    case OP_NEGATE: {
      if (!IS_NUMBER(peek(vm, 0))) {
        runtime_error("operand must be a number");
      }
      push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
      break;
    }

    // comparisons
    case OP_EQUAL: {
      b_value b = pop(vm);
      b_value a = pop(vm);
      push(vm, BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }

    case OP_NOT:
      push(vm, BOOL_VAL(is_falsey(pop(vm))));
      break;
    case OP_NIL:
      push(vm, NIL_VAL);
      break;
    case OP_TRUE:
      push(vm, BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(vm, BOOL_VAL(false));
      break;

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(vm, 0))) {
        frame->ip += offset;
      }
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_ECHO: {
      print_value(pop(vm));
      printf("\n"); // @TODO: Remove...
      break;
    }

    case OP_DUP: {
      push(vm, peek(vm, 0));
      break;
    }
    case OP_POP: {
      pop(vm);
      break;
    }
    case OP_POPN: {
      popn(vm, READ_SHORT());
      break;
    }
    case OP_CLOSE_UPVALUE: {
      close_upvalues(vm, vm->stack_top - 1);
      pop(vm);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      b_obj_string *name = READ_STRING();
      table_set(vm, &vm->globals, OBJ_VAL(name), peek(vm, 0));
      pop(vm);

#if DEBUG_MODE == 1
#if DEBUG_TABLE == 1
      table_print(&vm->globals);
#endif
#endif
      break;
    }

    case OP_GET_GLOBAL: {
      b_obj_string *name = READ_STRING();
      b_value value;
      if (!table_get(&vm->globals, OBJ_VAL(name), &value)) {
        runtime_error("%s is undefined in this scope", name->chars);
      }
      push(vm, value);
      break;
    }

    case OP_SET_GLOBAL: {
      b_obj_string *name = READ_STRING();
      if (table_set(vm, &vm->globals, OBJ_VAL(name), peek(vm, 0))) {
        table_delete(&vm->globals, OBJ_VAL(name));
        runtime_error("%s is undefined in this scope", name->chars);
      }
      break;
    }

    case OP_GET_LOCAL: {
      uint16_t slot = READ_SHORT();
      push(vm, frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint16_t slot = READ_SHORT();
      frame->slots[slot] = peek(vm, 0);
      break;
    }

    case OP_CLOSURE: {
      b_obj_func *function = AS_FUNCTION(READ_CONSTANT());
      b_obj_closure *closure = new_closure(vm, function);
      push(vm, OBJ_VAL(closure));

      for (int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        int index = READ_SHORT();

        if (is_local) {
          closure->upvalues[i] = capture_upvalue(vm, frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }

      break;
    }
    case OP_GET_UPVALUE: {
      int index = READ_SHORT();
      push(vm, *frame->closure->upvalues[index]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      int index = READ_SHORT();
      *frame->closure->upvalues[index]->location = peek(vm, 0);
      break;
    }

    case OP_CALL: {
      int arg_count = READ_BYTE();
      if (!call_value(vm, peek(vm, arg_count), arg_count)) {
        return PTR_RUNTIME_ERR;
      }
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    case OP_RETURN: {
      b_value result = pop(vm);

      close_upvalues(vm, frame->slots);

      vm->frame_count--;
      if (vm->frame_count == 0) {
        pop(vm);
        return PTR_OK;
      }

      vm->stack_top = frame->slots;
      push(vm, result);

      frame = &vm->frames[vm->frame_count - 1];
      break;
    }

    default:
      break;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_LCONSTANT
#undef READ_STRING
#undef READ_LSTRING
#undef BINARY_OP
#undef BINARY_MOD_OP
}

b_ptr_result interpret(b_vm *vm, const char *source) {
  b_blob blob;
  init_blob(&blob);

  b_obj_func *function = compile(vm, source, &blob);

  if (function == NULL) {
    free_blob(vm, &blob);
    return PTR_COMPILE_ERR;
  }

  push(vm, OBJ_VAL(function));
  b_obj_closure *closure = new_closure(vm, function);
  pop(vm);
  push(vm, OBJ_VAL(closure));
  call_value(vm, OBJ_VAL(closure), 0);

  b_ptr_result result = run(vm);

  return result;
}