#include "debug.h"
#include "object.h"
#include "value.h"

#include <stdio.h>

void disassemble_blob(b_blob *blob, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < blob->count;) {
    offset = disassemble_instruction(blob, offset);
  }
}

int simple_instruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int constant_instruction(const char *name, b_blob *blob, int offset) {
  uint16_t constant = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
  printf("%-16s %4d '", name, constant);
  print_value(blob->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

int short_instruction(const char *name, b_blob *blob, int offset) {
  uint16_t slot = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
  printf("%-16s %4d\n", name, slot);
  return offset + 3;
}

static int byte_instruction(const char *name, b_blob *blob, int offset) {
  uint8_t slot = blob->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int jump_instruction(const char *name, int sign, b_blob *blob,
                            int offset) {
  uint16_t jump = (uint16_t)(blob->code[offset + 1] << 8);
  jump |= blob->code[offset + 2];

  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

static int try_instruction(const char *name, b_blob *blob, int offset) {
  uint16_t jump = (uint16_t)(blob->code[offset + 1] << 8);
  jump |= blob->code[offset + 2];

  printf("%-16s %4d -> %d\n", name, offset, jump);
  return offset + 3;
}

static int invoke_instruction(const char *name, b_blob *blob, int offset) {
  uint16_t constant = (uint16_t)(blob->code[offset + 1] << 8);
  constant |= blob->code[offset + 2];
  uint8_t arg_count = blob->code[offset + 3];

  printf("%-16s (%d args) %4d '", name, arg_count, constant);
  print_value(blob->constants.values[constant]);
  printf("'\n");
  return offset + 4;
}

static int method_instruction(const char *name, b_blob *blob, int offset) {
  uint16_t constant = (uint16_t)(blob->code[offset + 1] << 8);
  constant |= blob->code[offset + 2];
  uint8_t is_static = blob->code[offset + 3];

  printf("%-16s %4d '", name, constant);
  print_value(blob->constants.values[constant]);
  printf("' %d\n", is_static);
  return offset + 4;
}

int disassemble_instruction(b_blob *blob, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && blob->lines[offset] == blob->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", blob->lines[offset]);
  }

  uint8_t instruction = blob->code[offset];
  switch (instruction) {
  case OP_JUMP_IF_FALSE:
    return jump_instruction("fjump", 1, blob, offset);
  case OP_JUMP:
    return jump_instruction("jump", 1, blob, offset);
  case OP_TRY:
    return try_instruction("itry", blob, offset);
  case OP_LOOP:
    return jump_instruction("loop", -1, blob, offset);

  case OP_DEFINE_GLOBAL:
    return constant_instruction("dglob", blob, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("gglob", blob, offset);
  case OP_SET_GLOBAL:
    return constant_instruction("sglob", blob, offset);

  case OP_GET_LOCAL:
    return short_instruction("gloc", blob, offset);
  case OP_SET_LOCAL:
    return short_instruction("sloc", blob, offset);

  case OP_GET_PROPERTY:
    return constant_instruction("gprop", blob, offset);
  case OP_SET_PROPERTY:
    return constant_instruction("sprop", blob, offset);

  case OP_GET_UPVALUE:
    return short_instruction("gupv", blob, offset);
  case OP_SET_UPVALUE:
    return short_instruction("supv", blob, offset);

  case OP_END_TRY:
    return simple_instruction("etry", offset);

  case OP_CONSTANT:
    return constant_instruction("load", blob, offset);

  case OP_EQUAL:
    return simple_instruction("eq", offset);

  case OP_GREATER:
    return simple_instruction("gt", offset);
  case OP_LESS:
    return simple_instruction("less", offset);
  case OP_NIL:
    return simple_instruction("nil", offset);
  case OP_EMPTY:
    return simple_instruction("empty", offset);
  case OP_TRUE:
    return simple_instruction("true", offset);
  case OP_FALSE:
    return simple_instruction("false", offset);
  case OP_ADD:
    return simple_instruction("add", offset);
  case OP_SUBTRACT:
    return simple_instruction("sub", offset);
  case OP_MULTIPLY:
    return simple_instruction("mult", offset);
  case OP_DIVIDE:
    return simple_instruction("div", offset);
  case OP_FDIVIDE:
    return simple_instruction("fdiv", offset);
  case OP_REMINDER:
    return simple_instruction("rmod", offset);
  case OP_POW:
    return simple_instruction("pow", offset);
  case OP_NEGATE:
    return simple_instruction("neg", offset);
  case OP_NOT:
    return simple_instruction("not", offset);
  case OP_BIT_NOT:
    return simple_instruction("bnot", offset);
  case OP_AND:
    return simple_instruction("band", offset);
  case OP_OR:
    return simple_instruction("bor", offset);
  case OP_XOR:
    return simple_instruction("bxor", offset);
  case OP_LSHIFT:
    return simple_instruction("lshift", offset);
  case OP_RSHIFT:
    return simple_instruction("rshift", offset);
  case OP_ONE:
    return simple_instruction("one", offset);

  case OP_CALL_IMPORT:
    return short_instruction("cimport", blob, offset);
  case OP_FINISH_MODULE:
    return short_instruction("fimport", blob, offset);

  case OP_ECHO:
    return simple_instruction("echo", offset);
  case OP_DIE:
    return simple_instruction("die", offset);
  case OP_POP:
    return simple_instruction("pop", offset);
  case OP_CLOSE_UPVALUE:
    return simple_instruction("clupv", offset);
  case OP_DUP:
    return simple_instruction("dup", offset);
  case OP_ASSERT:
    return simple_instruction("assrt", offset);
  case OP_POPN:
    return short_instruction("popn", blob, offset);

    // data container manipulators
  case OP_RANGE:
    return short_instruction("range", blob, offset);
  case OP_LIST:
    return short_instruction("list", blob, offset);
  case OP_DICT:
    return short_instruction("dict", blob, offset);
  case OP_GET_INDEX:
    return byte_instruction("gindx", blob, offset);
  case OP_SET_INDEX:
    return simple_instruction("sindx", offset);

  case OP_CLOSURE: {
    offset++;
    uint16_t constant = blob->code[offset++] << 8;
    constant |= blob->code[offset++];
    printf("%-16s %4d ", "clsur", constant);
    print_value(blob->constants.values[constant]);
    printf("\n");

    b_obj_func *function = AS_FUNCTION(blob->constants.values[constant]);
    for (int j = 0; j < function->upvalue_count; j++) {
      int is_local = blob->code[offset++];
      uint16_t index = blob->code[offset++] << 8;
      index |= blob->code[offset++];
      printf("%04d      |                     %s %d\n", offset - 3,
             is_local ? "local" : "upvalue", (int)index);
    }

    return offset;
  }
  case OP_CALL:
    return byte_instruction("call", blob, offset);
  case OP_INVOKE:
    return invoke_instruction("invk", blob, offset);
  case OP_RETURN:
    return simple_instruction("ret", offset);

  case OP_CLASS:
    return constant_instruction("class", blob, offset);
  case OP_METHOD:
    return method_instruction("methd", blob, offset);
  case OP_CLASS_PROPERTY:
    return method_instruction("clprop", blob, offset);
  /* case OP_CLASS_PROPERTY:
    return constant_instruction("clprop", blob, offset); */
  case OP_GET_SUPER:
    return constant_instruction("gsup", blob, offset);
  case OP_INHERIT:
    return simple_instruction("inher", offset);
  case OP_SUPER_INVOKE:
    return invoke_instruction("sinvk", blob, offset);

  default:
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
  }
}