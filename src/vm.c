#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "config.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#if DEBUG_MODE == 1
#include "debug.h"
#endif

#define runtime_error(...)                                                     \
  _runtime_error(vm, ##__VA_ARGS__);                                           \
  return PTR_RUNTIME_ERR

static void reset_stack(b_vm *vm) { vm->stack_top = vm->stack; }

void _runtime_error(b_vm *vm, const char *format, ...) {

  size_t instruction = vm->ip - vm->blob->code - 1;
  int line = vm->blob->lines[instruction];

  fprintf(stderr, "RuntimeError:\n");
  fprintf(stderr, "    File: <script>, Line: %d\n    Message: ", line);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  reset_stack(vm);
}

void init_vm(b_vm *vm) {
  reset_stack(vm);
  vm->objects = NULL;
  init_table(&vm->strings);
  init_table(&vm->globals);
}

void free_vm(b_vm *vm) {
  free_objects(vm);
  free_table(&vm->strings);
  free_table(&vm->globals);
}

void push(b_vm *vm, b_value value) {
  *vm->stack_top = value;
  vm->stack_top++;
}

b_value pop(b_vm *vm) {
  vm->stack_top--;
  return *vm->stack_top;
}

static b_value peek(b_vm *vm, int distance) {
  return vm->stack_top[-1 - distance];
}

static bool is_falsey(b_value value) {
  if (IS_BOOL(value))
    return IS_BOOL(value) && !AS_BOOL(value);
  if (IS_NIL(value))
    return true;

  // -1 is the number equivalent of false in Birdy
  if (IS_NUMBER(value))
    return AS_NUMBER(value) < 0;

  /* // Non-empty strings are true, empty strings are false.
  if (IS_STRING(value))
    return strlen(AS_STRING(value)->chars) < 1;

  // Non-empty lists are true, empty lists are false.
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
  b_value _b = pop(vm);
  b_value _a = pop(vm);

  if (IS_NIL(_a)) {
    push(vm, _b);
  } else if (IS_NIL(_b)) {
    push(vm, _a);
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

#define READ_BYTE() (*vm->ip++)

#define READ_SHORT() (vm->ip += 2, (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]))

#define READ_CONSTANT() (vm->blob->constants.values[READ_BYTE()])

#define READ_LCONSTANT()                                                       \
  (vm->blob->constants.values[(READ_BYTE() << 8) | READ_BYTE()])

#define READ_STRING() (AS_STRING(READ_CONSTANT()))

#define READ_LSTRING() (AS_STRING(READ_LCONSTANT()))

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

#ifdef DEBUG_TRACE_EXECUTION
#if DEBUG_TRACE_EXECUTION == 1
    printf("          ");
    for (b_value *slot = vm->stack; slot < vm->stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(vm->blob, (int)(vm->ip - vm->blob->code));
#endif
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT:
    case OP_LCONSTANT: {
      b_value constant =
          instruction == OP_CONSTANT ? READ_CONSTANT() : READ_LCONSTANT();
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

    case OP_ECHO: {
      print_value(pop(vm));
      printf("\n"); // @TODO: Remove...
      break;
    }

    case OP_POP: {
      pop(vm);
      break;
    }

    case OP_DEFINE_GLOBAL:
    case OP_DEFINE_LGLOBAL: {
      b_obj_string *name =
          instruction == OP_DEFINE_GLOBAL ? READ_STRING() : READ_LSTRING();
      table_set(&vm->globals, OBJ_VAL(name), peek(vm, 0));
      pop(vm);

#if DEBUG_MODE == 1
#if DEBUG_TABLE == 1
      table_print(&vm->globals);
#endif
#endif
      break;
    }

    case OP_GET_GLOBAL:
    case OP_GET_LGLOBAL: {
      b_obj_string *name =
          instruction == OP_GET_GLOBAL ? READ_STRING() : READ_LSTRING();
      b_value value;
      if (!table_get(&vm->globals, OBJ_VAL(name), &value)) {
        runtime_error("%s is undefined in this scope", name->chars);
      }
      push(vm, value);
      break;
    }

    case OP_SET_GLOBAL:
    case OP_SET_LGLOBAL: {
      b_obj_string *name =
          instruction == OP_SET_GLOBAL ? READ_STRING() : READ_LSTRING();
      if (table_set(&vm->globals, OBJ_VAL(name), peek(vm, 0))) {
        table_delete(&vm->globals, OBJ_VAL(name));
        runtime_error("%s is undefined in this scope", name->chars);
      }
      break;
    }

    case OP_GET_LOCAL:
    case OP_GET_LLOCAL: {
      uint16_t slot = instruction == OP_GET_LOCAL ? READ_BYTE() : READ_SHORT();
      push(vm, vm->stack[slot]);
      break;
    }
    case OP_SET_LOCAL:
    case OP_SET_LLOCAL: {
      uint16_t slot = instruction == OP_SET_LOCAL ? READ_BYTE() : READ_SHORT();
      vm->stack[slot] = peek(vm, 0);
      break;
    }

    case OP_RETURN: {
      // print_value(pop(vm));
      // printf("\n");
      return PTR_OK;
    }

    default:
      break;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef BINARY_MOD_OP
}

b_ptr_result interpret(b_vm *vm, const char *source) {
  b_blob blob;
  init_blob(&blob);

  if (!compile(vm, source, &blob)) {
    free_blob(&blob);
    return PTR_COMPILE_ERR;
  }

  vm->blob = &blob;
  vm->ip = vm->blob->code;

  b_ptr_result result = run(vm);

  free_blob(&blob);

  return result;
}