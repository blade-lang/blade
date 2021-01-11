#ifndef bird_compiler_h
#define bird_compiler_h

#include "scanner.h"
#include "vm.h"

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =, >=, <=, &=, |=, *=, +=, -=, /=, **=, %=, >>=, <<=, ^=,
                   // ~=
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // ==, !=
  PREC_COMPARISON, // <, >, <=, >=
  PREC_BIT_OR,     // |
  PREC_BIT_XOR,    // ^
  PREC_BIT_AND,    // &
  PREC_SHIFT,      // <<, >>
  PREC_RANGE,      // ..
  PREC_TERM,       // +, -
  PREC_FACTOR,     // *, /, %, **, //
  PREC_UNARY, // !, -, ~, (++, -- this two will now be treated as statements)
  PREC_CALL,  // ., ()
  PREC_PRIMARY
} b_prec;

typedef struct {
  b_token name;
  int depth;
} b_local;

typedef struct {
  b_local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} b_compiler;

typedef struct {
  b_scanner *scanner;
  b_vm *vm;

  b_token current;
  b_token previous;
  bool had_error;
  bool panic_mode;
  b_blob *current_blob;
  b_compiler *compiler;
} b_parser;

typedef void (*b_parse_fn)(b_parser *, bool);

typedef struct {
  b_parse_fn prefix;
  b_parse_fn infix;
  b_prec precedence;
} b_parse_rule;

bool compile(b_vm *vm, const char *source, b_blob *blob);

#endif