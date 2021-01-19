#ifndef bird_compiler_h
#define bird_compiler_h

#include "object.h"
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

typedef enum {
  TYPE_FUNCTION,
  TYPE_METHOD,
  TYPE_INITIALIZER,
  TYPE_SCRIPT,
} b_func_type;

typedef struct {
  b_token name;
  int depth;
  bool is_captured;
} b_local;

typedef struct {
  uint16_t index;
  bool is_local;
} b_upvalue;

struct s_compiler {
  b_compiler *enclosing;

  // current function
  b_obj_func *function;
  b_func_type type;

  b_local locals[UINT8_COUNT];
  int local_count;
  b_upvalue upvalues[UINT8_COUNT];
  int scope_depth;
};

typedef struct b_class_compiler {
  struct b_class_compiler *enclosing;
  b_token name;
} b_class_compiler;

typedef struct {
  b_scanner *scanner;
  b_vm *vm;

  b_token current;
  b_token previous;
  bool had_error;
  bool panic_mode;
  bool in_block;
  b_compiler *compiler;
  b_class_compiler *current_class;

  // used for tracking loops for the continue statement...
  int innermost_loop_start;
  int innermost_loop_scope_depth;
} b_parser;

typedef void (*b_parse_fn)(b_parser *, bool);

typedef struct {
  b_parse_fn prefix;
  b_parse_fn infix;
  b_prec precedence;
} b_parse_rule;

b_obj_func *compile(b_vm *vm, const char *source, b_blob *blob);
void mark_compiler_roots(b_vm *vm);

#endif