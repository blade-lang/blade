#ifndef bird_blob_h
#define bird_blob_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,

  OP_GET_LOCAL,
  OP_SET_LOCAL,

  OP_JUMP_IF_FALSE,
  OP_JUMP,
  OP_LOOP,

  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_FDIVIDE, // floor divide
  OP_REMINDER,
  OP_POW,
  OP_NEGATE,
  OP_NOT,
  OP_CONSTANT, // 8-bit constant address (0 - 255)
  OP_ECHO,
  OP_POP,
  OP_DUP,
  OP_POPN,

  OP_CALL,
  OP_RETURN,

  // the break placeholder... it never gets to the vm
  // care should be taken to
  OP_BREAK_PL,
} b_code;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  int *lines;
  b_value_arr constants;
} b_blob;

void init_blob(b_blob *blob);
void free_blob(b_blob *blob);
void write_blob(b_blob *blob, uint8_t byte, int line);
int add_constant(b_blob *blob, b_value value);

#endif