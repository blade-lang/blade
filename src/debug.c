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

int disassemble_instruction(b_blob *blob, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && blob->lines[offset] == blob->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", blob->lines[offset]);
  }

  uint8_t instruction = blob->code[offset];
  switch (instruction) {
  case OP_LONG_CONSTANT:
    return long_constant_instruction("lload", blob, offset);
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
  case OP_RETURN:
    return simple_instruction("ret", offset);

  default:
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
  }
}