#include <stdio.h>

#include "debug.h"
#include "value.h"

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
  uint8_t constant = blob->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  print_value(blob->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int long_constant_instruction(const char *name, b_blob *blob, int offset) {
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

  case OP_DEFINE_GLOBAL:
    return constant_instruction("dglob", blob, offset);
  case OP_DEFINE_LGLOBAL:
    return long_constant_instruction("dlglob", blob, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("gglob", blob, offset);
  case OP_GET_LGLOBAL:
    return long_constant_instruction("glglob", blob, offset);
  case OP_SET_GLOBAL:
    return constant_instruction("sglob", blob, offset);
  case OP_SET_LGLOBAL:
    return long_constant_instruction("slglob", blob, offset);

  case OP_GET_LOCAL:
    return byte_instruction("gloc", blob, offset);
  case OP_SET_LOCAL:
    return byte_instruction("sloc", blob, offset);
  case OP_GET_LLOCAL:
    return short_instruction("lgloc", blob, offset);
  case OP_SET_LLOCAL:
    return short_instruction("lsloc", blob, offset);

  case OP_CONSTANT:
    return constant_instruction("load", blob, offset);
  case OP_LCONSTANT:
    return long_constant_instruction("lload", blob, offset);

  case OP_EQUAL:
    return simple_instruction("eq", offset);

  case OP_GREATER:
    return simple_instruction("gt", offset);
  case OP_LESS:
    return simple_instruction("less", offset);
  case OP_NIL:
    return simple_instruction("nil", offset);
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

  case OP_ECHO:
    return simple_instruction("echo", offset);
  case OP_POP:
    return simple_instruction("pop", offset);
  case OP_POPN:
    return short_instruction("popn", blob, offset);

  case OP_RETURN:
    return simple_instruction("ret", offset);

  default:
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
  }
}