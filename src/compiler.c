#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "config.h"
#include "object.h"
#include "scanner.h"
#include "util.h"

#if DEBUG_MODE == 1
#include "debug.h"
#endif

static void error_at(b_parser *p, b_token *t, const char *message,
                     va_list args) {
  // do not cascade error
  // suppress error if already in panic mode
  if (p->panic_mode)
    return;

  p->panic_mode = true;

  fprintf(stderr, "SyntaxError:\n");
  fprintf(stderr, "    File: <script>, Line: %d\n", t->line);

  fprintf(stderr, "    Error");

  if (t->type == EOF_TOKEN) {
    fprintf(stderr, " at end");
  } else if (t->type == ERROR_TOKEN) {
    // do nothing
  } else {
    if (t->length == 1 && *t->start == '\n') {
      fprintf(stderr, " at newline");
    } else {
      fprintf(stderr, " at '%.*s'", t->length, t->start);
    }
  }

  fprintf(stderr, ": ");
  vfprintf(stderr, message, args);
  fputs("\n", stderr);

  p->had_error = true;
}

static void error(b_parser *p, const char *message, ...) {
  va_list args;
  va_start(args, message);
  error_at(p, &p->previous, message, args);
}

static void error_at_current(b_parser *p, const char *message, ...) {
  va_list args;
  va_start(args, message);
  error_at(p, &p->current, message, args);
}

static void advance(b_parser *p) {
  p->previous = p->current;

  for (;;) {
    p->current = scan_token(p->scanner);
    if (p->current.type != ERROR_TOKEN)
      break;

    error_at_current(p, p->current.start);
  }
}

static void consume(b_parser *p, b_tkn_type t, const char *message) {
  if (p->current.type == t) {
    advance(p);
    return;
  }

  error_at_current(p, message);
}

static bool check(b_parser *p, b_tkn_type t) { return p->current.type == t; }

static bool match(b_parser *p, b_tkn_type t) {
  if (!check(p, t))
    return false;
  advance(p);
  return true;
}

static void consume_statement_end(b_parser *p) {
  if (match(p, SEMICOLON_TOKEN) || match(p, EOF_TOKEN)) {
    while (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN))
      ;
    return;
  }

  consume(p, NEWLINE_TOKEN, "end of statement expected");
  while (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN))
    ;
}

static void ignore_whitespace(b_parser *p) {
  while (match(p, NEWLINE_TOKEN))
    ;
}

static void emit_byte(b_parser *p, uint8_t byte) {
  write_blob(p->current_blob, byte, p->previous.line);
}

static void emit_bytes(b_parser *p, uint8_t byte, uint8_t byte2) {
  write_blob(p->current_blob, byte, p->previous.line);
  write_blob(p->current_blob, byte2, p->previous.line);
}

static void emit_byte_and_long(b_parser *p, uint8_t byte, uint16_t byte2) {
  write_blob(p->current_blob, byte, p->previous.line);
  write_blob(p->current_blob, (byte2 >> 8) & 0xff, p->previous.line);
  write_blob(p->current_blob, byte2 & 0xff, p->previous.line);
}

static void emit_return(b_parser *p) { emit_byte(p, OP_RETURN); }

static int make_constant(b_parser *p, b_value value) {
  int constant = add_constant(p->current_blob, value);
  if (constant >= UINT16_MAX) {
    error(p, "too many constants in current scope");
    return 0;
  }
  return constant;
}

static void emit_constant(b_parser *p, b_value value) {
  int constant = make_constant(p, value);
  if (constant <= UINT8_MAX) {
    emit_bytes(p, OP_CONSTANT, (uint8_t)constant);
  } else {
    emit_bytes(p, OP_LCONSTANT, (uint16_t)constant);
  }
}

static void init_compiler(b_parser *p, b_compiler *compiler) {
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  p->compiler = compiler;
}

static int identifier_constant(b_parser *p, b_token *name) {
  return make_constant(p,
                       OBJ_VAL(copy_string(p->vm, name->start, name->length)));
}

static bool identifiers_equal(b_token *a, b_token *b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(b_parser *p, b_token *name) {
  for (int i = p->compiler->local_count - 1; i >= 0; i--) {
    b_local *local = &p->compiler->locals[i];
    if (identifiers_equal(&local->name, name)) {
      if (local->depth == -1) {
        error(p, "cannot read local variable in it's own initializer");
      }
      return i;
    }
  }
  return -1;
}

static void add_local(b_parser *p, b_token name) {
  if (p->compiler->local_count == UINT8_COUNT) {
    // we've reached maximum local variables per scope
    error(p, "too many local variables in scope");
    return;
  }

  b_local *local = &p->compiler->locals[p->compiler->local_count++];
  local->name = name;
  // local->depth = p->compiler->scope_depth;
  local->depth = -1;
}

static void declare_variable(b_parser *p) {
  // global variables are implicitly declared...
  if (p->compiler->scope_depth == 0)
    return;

  b_token *name = &p->previous;

  for (int i = p->compiler->local_count - 1; i >= 0; i--) {
    b_local *local = &p->compiler->locals[i];
    if (local->depth != -1 && local->depth < p->compiler->scope_depth) {
      break;
    }

    if (identifiers_equal(name, &local->name)) {
      error(p, "%.*s already declared in current scope", name->length,
            name->start);
    }
  }

  add_local(p, *name);
}

static void end_compiler(b_parser *p) {
  emit_return(p);

#ifdef DEBUG_PRINT_CODE
#if DEBUG_PRINT_CODE == 1
  if (!p->had_error) {
    disassemble_blob(p->current_blob, "code");
  }
#endif
#endif
}

static void begin_scope(b_parser *p) { p->compiler->scope_depth++; }

static void end_scope(b_parser *p) {
  p->compiler->scope_depth--;

  // remove all variables declared in scope while exiting...
  while (p->compiler->local_count > 0 &&
         p->compiler->locals[p->compiler->local_count - 1].depth >
             p->compiler->scope_depth) {
    emit_byte(p, OP_POP);
    p->compiler->local_count--;
  }
}

// --> Forward declarations start
static void expression(b_parser *p);
static void statement(b_parser *p);
static void declaration(b_parser *p);
static b_parse_rule *get_rule(b_tkn_type type);
static void parse_precedence(b_parser *p, b_prec precedence);
// --> Forward declarations end

static void binary(b_parser *p, bool can_assign) {
  b_tkn_type op = p->previous.type;

  // compile the right operand
  b_parse_rule *rule = get_rule(op);
  parse_precedence(p, (b_prec)(rule->precedence + 1));

  // emit the operator instruction
  switch (op) {
  case PLUS_TOKEN:
    emit_byte(p, OP_ADD);
    break;
  case MINUS_TOKEN:
    emit_byte(p, OP_SUBTRACT);
    break;
  case MULTIPLY_TOKEN:
    emit_byte(p, OP_MULTIPLY);
    break;
  case DIVIDE_TOKEN:
    emit_byte(p, OP_DIVIDE);
    break;
  case PERCENT_TOKEN:
    emit_byte(p, OP_REMINDER);
    break;
  case POW_TOKEN:
    emit_byte(p, OP_POW);
    break;
  case FLOOR_TOKEN:
    emit_byte(p, OP_FDIVIDE);
    break;

  // equality
  case EQUAL_EQ_TOKEN:
    emit_byte(p, OP_EQUAL);
    break;
  case BANG_EQ_TOKEN:
    emit_bytes(p, OP_EQUAL, OP_NOT);
    break;
  case GREATER_TOKEN:
    emit_byte(p, OP_GREATER);
    break;
  case GREATER_EQ_TOKEN:
    emit_bytes(p, OP_LESS, OP_NOT);
    break;
  case LESS_TOKEN:
    emit_byte(p, OP_LESS);
    break;
  case LESS_EQ_TOKEN:
    emit_bytes(p, OP_GREATER, OP_NOT);
    break;

  default:
    break;
  }
}

static void literal(b_parser *p, bool can_assign) {
  switch (p->previous.type) {
  case NIL_TOKEN:
    emit_byte(p, OP_NIL);
    break;
  case TRUE_TOKEN:
    emit_byte(p, OP_TRUE);
    break;
  case FALSE_TOKEN:
    emit_byte(p, OP_FALSE);
    break;
  default:
    return;
  }
}

static void named_variable(b_parser *p, b_token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(p, &name);
  if (arg != -1) {
    get_op = arg <= UINT8_MAX ? OP_GET_LOCAL : OP_GET_LLOCAL;
    set_op = arg <= UINT8_MAX ? OP_SET_LOCAL : OP_SET_LLOCAL;
  } else {
    arg = identifier_constant(p, &name);
    get_op = arg <= UINT8_MAX ? OP_GET_GLOBAL : OP_GET_LGLOBAL;
    set_op = arg <= UINT8_MAX ? OP_SET_GLOBAL : OP_SET_LGLOBAL;
  }

  if (can_assign && match(p, EQUAL_TOKEN)) {
    expression(p);
    if (arg <= UINT8_MAX) {
      emit_bytes(p, set_op, (uint8_t)arg);
    } else {
      emit_byte_and_long(p, set_op, (uint16_t)arg);
    }
  } else {
    if (arg <= UINT8_MAX) {
      emit_bytes(p, get_op, (uint8_t)arg);
    } else {
      emit_byte_and_long(p, get_op, (uint16_t)arg);
    }
  }
}

static void variable(b_parser *p, bool can_assign) {
  named_variable(p, p->previous, can_assign);
}

static void grouping(b_parser *p, bool can_assign) {
  expression(p);
  consume(p, RPAREN_TOKEN, "expected ')' after grouped expression");
}

static void number(b_parser *p, bool can_assign) {
  if (p->previous.type == BIN_NUMBER_TOKEN) {
    long long value = strtoll(p->previous.start + 2, NULL, 2);
    emit_constant(p, NUMBER_VAL(value));
  } else if (p->previous.type == OCT_NUMBER_TOKEN) {
    long value = strtol(p->previous.start + 2, NULL, 8);
    emit_constant(p, NUMBER_VAL(value));
  } else if (p->previous.type == HEX_NUMBER_TOKEN) {
    long value = strtol(p->previous.start, NULL, 16);
    emit_constant(p, NUMBER_VAL(value));
  } else {
    double value = strtod(p->previous.start, NULL);
    emit_constant(p, NUMBER_VAL(value));
  }
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int read_hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return -1;
}

// Reads [digits] hex digits in a string literal and returns their number value.
static int read_hex_escape(b_parser *p, char *str, int index, int count) {
  int value = 0;
  int i = 0, digit = 0;
  for (; i < count; i++) {
    int digit = read_hex_digit(str[index + i + 2]);
    if (digit == -1) {
      error(p, "invalid escape sequence");
    }
    value = (value * 16) | digit;
  }
  if (count == 4 && (digit = read_hex_digit(str[index + i + 2])) != -1) {
    value = (value * 16) | digit;
  }
  return value;
}

static int read_unicode_escape(b_parser *p, char *string, char *real_string,
                               int number_bytes, int real_index, int index) {
  int value = read_hex_escape(p, real_string, real_index, number_bytes);
  int count = utf8_number_bytes(value);
  if (count == -1) {
    error(p, "cannot encode a negative unicode value");
  }
  if (value > 65535) // check for greater that \uffff
    count++;
  if (count != 0) {
    memcpy(string + index, utf8_encode(value), count + 1);
  }
  /* if (value > 65535) // but greater than \uffff doesn't occupy any extra byte
    count--; */
  return count;
}

static void string(b_parser *p, bool can_assign) {
  char *str = (char *)calloc(p->previous.length - 2, sizeof(char));
  char *real = (char *)(p->previous.start + 1);

  int real_length = p->previous.length - 2;
  int i = 0, k = 0;

  for (; i < real_length; i++, k++) {
    char c = real[i];
    if (c == '\\' && i < real_length - 1) {
      switch (real[i + 1]) {
      case '0':
        c = '\0';
        break;
      case '\'':
        c = '\'';
        break;
      case '"':
        c = '"';
        break;
      case 'a':
        c = '\a';
        break;
      case 'b':
        c = '\b';
        break;
      case 'f':
        c = '\f';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case '\\':
        c = '\\';
        break;
      case 'v':
        c = '\v';
        break;
      case 'x': {
        k += read_unicode_escape(p, str, real, 2, i, k) - 1;
        i += 3;
        continue;
      }
      case 'u': {
        int count = read_unicode_escape(p, str, real, 4, i, k);
        k += count > 4 ? count - 2 : count - 1;
        i += count > 4 ? 6 : 5;
        continue;
      }
      case 'U': {
        k += read_unicode_escape(p, str, real, 8, i, k) - 1;
        i += 9;
        continue;
      }
      default:
        i--;
        break;
      }
      i++;
    }
    memcpy(str + k, &c, 1);
  }

  emit_constant(p, OBJ_VAL(copy_string(p->vm, str, k)));
}

static void unary(b_parser *p, bool can_assign) {
  b_tkn_type op = p->previous.type;

  // compile the expression
  parse_precedence(p, PREC_UNARY);

  // emit instruction
  switch (op) {
  case MINUS_TOKEN:
    emit_byte(p, OP_SUBTRACT);
    break;
  case BANG_TOKEN:
    emit_byte(p, OP_NOT);
    break;

  default:
    break;
  }
}

b_parse_rule parse_rules[] = {
    // symbols
    [NEWLINE_TOKEN] = {NULL, NULL, PREC_NONE},            // (
    [LPAREN_TOKEN] = {grouping, NULL, PREC_NONE},         // (
    [RPAREN_TOKEN] = {NULL, NULL, PREC_NONE},             // )
    [LBRACKET_TOKEN] = {NULL, NULL, PREC_NONE},           // [
    [RBRACKET_TOKEN] = {NULL, NULL, PREC_NONE},           // ]
    [LBRACE_TOKEN] = {NULL, NULL, PREC_NONE},             // {
    [RBRACE_TOKEN] = {NULL, NULL, PREC_NONE},             // }
    [SEMICOLON_TOKEN] = {NULL, NULL, PREC_NONE},          // ;
    [COMMA_TOKEN] = {NULL, NULL, PREC_NONE},              // ,
    [BACKSLASH_TOKEN] = {NULL, NULL, PREC_NONE},          // '\'
    [BANG_TOKEN] = {unary, NULL, PREC_NONE},              // !
    [BANG_EQ_TOKEN] = {NULL, binary, PREC_EQUALITY},      // !=
    [COLON_TOKEN] = {NULL, NULL, PREC_NONE},              // :
    [AT_TOKEN] = {NULL, NULL, PREC_NONE},                 // @
    [DOT_TOKEN] = {NULL, NULL, PREC_NONE},                // .
    [RANGE_TOKEN] = {NULL, NULL, PREC_NONE},              // ..
    [TRIDOT_TOKEN] = {NULL, NULL, PREC_NONE},             // ...
    [PLUS_TOKEN] = {NULL, binary, PREC_TERM},             // +
    [PLUS_EQ_TOKEN] = {NULL, NULL, PREC_NONE},            // +=
    [INCREMENT_TOKEN] = {NULL, NULL, PREC_NONE},          // ++
    [MINUS_TOKEN] = {unary, binary, PREC_TERM},           // -
    [MINUS_EQ_TOKEN] = {NULL, NULL, PREC_NONE},           // -=
    [DECREMENT_TOKEN] = {NULL, NULL, PREC_NONE},          // --
    [MULTIPLY_TOKEN] = {NULL, binary, PREC_FACTOR},       // *
    [MULTIPLY_EQ_TOKEN] = {NULL, NULL, PREC_NONE},        // *=
    [POW_TOKEN] = {NULL, binary, PREC_FACTOR},            // **
    [POW_EQ_TOKEN] = {NULL, NULL, PREC_NONE},             // **=
    [DIVIDE_TOKEN] = {NULL, binary, PREC_FACTOR},         // '/'
    [DIVIDE_EQ_TOKEN] = {NULL, NULL, PREC_NONE},          // '/='
    [FLOOR_TOKEN] = {NULL, binary, PREC_FACTOR},          // '//'
    [FLOOR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},           // '//='
    [EQUAL_TOKEN] = {NULL, NULL, PREC_NONE},              // =
    [EQUAL_EQ_TOKEN] = {NULL, binary, PREC_EQUALITY},     // ==
    [LESS_TOKEN] = {NULL, binary, PREC_COMPARISON},       // <
    [LESS_EQ_TOKEN] = {NULL, binary, PREC_COMPARISON},    // <=
    [LSHIFT_TOKEN] = {NULL, NULL, PREC_NONE},             // <<
    [LSHIFT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},          // <<=
    [GREATER_TOKEN] = {NULL, binary, PREC_COMPARISON},    // >
    [GREATER_EQ_TOKEN] = {NULL, binary, PREC_COMPARISON}, // >=
    [RSHIFT_TOKEN] = {NULL, NULL, PREC_NONE},             // >>
    [RSHIFT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},          // >>=
    [PERCENT_TOKEN] = {NULL, binary, PREC_FACTOR},        // %
    [PERCENT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},         // %=
    [AMP_TOKEN] = {NULL, NULL, PREC_NONE},                // &
    [AMP_EQ_TOKEN] = {NULL, NULL, PREC_NONE},             // &=
    [BAR_TOKEN] = {NULL, NULL, PREC_NONE},                // |
    [BAR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},             // |=
    [TILDE_TOKEN] = {NULL, NULL, PREC_NONE},              // ~
    [TILDE_EQ_TOKEN] = {NULL, NULL, PREC_NONE},           // ~=
    [XOR_TOKEN] = {NULL, NULL, PREC_NONE},                // ^
    [XOR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},             // ^=
    [CDEFAULT_TOKEN] = {NULL, NULL, PREC_NONE},           // ??

    // keywords
    [AND_TOKEN] = {NULL, NULL, PREC_NONE},
    [AS_TOKEN] = {NULL, NULL, PREC_NONE},
    [ASSERT_TOKEN] = {NULL, NULL, PREC_NONE},
    [BREAK_TOKEN] = {NULL, NULL, PREC_NONE},
    [CLASS_TOKEN] = {NULL, NULL, PREC_NONE},
    [CONTINUE_TOKEN] = {NULL, NULL, PREC_NONE},
    [DEF_TOKEN] = {NULL, NULL, PREC_NONE},
    [DEFAULT_TOKEN] = {NULL, NULL, PREC_NONE},
    [DIE_TOKEN] = {NULL, NULL, PREC_NONE},
    [ECHO_TOKEN] = {NULL, NULL, PREC_NONE},
    [ELSE_TOKEN] = {NULL, NULL, PREC_NONE},
    [FALSE_TOKEN] = {literal, NULL, PREC_NONE},
    [FOR_TOKEN] = {NULL, NULL, PREC_NONE},
    [IF_TOKEN] = {NULL, NULL, PREC_NONE},
    [IMPORT_TOKEN] = {NULL, NULL, PREC_NONE},
    [IN_TOKEN] = {NULL, NULL, PREC_NONE},
    [ITER_TOKEN] = {NULL, NULL, PREC_NONE},
    [VAR_TOKEN] = {NULL, NULL, PREC_NONE},
    [NIL_TOKEN] = {literal, NULL, PREC_NONE},
    [OR_TOKEN] = {NULL, NULL, PREC_NONE},
    [PARENT_TOKEN] = {NULL, NULL, PREC_NONE},
    [RETURN_TOKEN] = {NULL, NULL, PREC_NONE},
    [SELF_TOKEN] = {NULL, NULL, PREC_NONE},
    [STATIC_TOKEN] = {NULL, NULL, PREC_NONE},
    [TRUE_TOKEN] = {literal, NULL, PREC_NONE},
    [USING_TOKEN] = {NULL, NULL, PREC_NONE},
    [WHEN_TOKEN] = {NULL, NULL, PREC_NONE},
    [WHILE_TOKEN] = {NULL, NULL, PREC_NONE},

    // types token
    [LITERAL_TOKEN] = {string, NULL, PREC_NONE},
    [REG_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // regular numbers
    [BIN_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // binary numbers
    [OCT_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // octal numbers
    [HEX_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // hexadecimal numbers
    [IDENTIFIER_TOKEN] = {variable, NULL, PREC_NONE},
    [EOF_TOKEN] = {NULL, NULL, PREC_NONE},

    // error
    [ERROR_TOKEN] = {NULL, NULL, PREC_NONE},
    [EMPTY_TOKEN] = {NULL, NULL, PREC_NONE},
};

static void parse_precedence(b_parser *p, b_prec precedence) {
  advance(p);

  b_parse_fn prefix_rule = get_rule(p->previous.type)->prefix;

  if (prefix_rule == NULL) {
    error(p, "expected expression");
    return;
  }

  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(p, can_assign);

  while (precedence <= get_rule(p->current.type)->precedence) {
    advance(p);
    b_parse_fn infix_rule = get_rule(p->previous.type)->infix;
    infix_rule(p, can_assign);
  }

  if (can_assign && match(p, EQUAL_TOKEN)) {
    error(p, "invalid assignment target");
  }
}

static b_parse_rule *get_rule(b_tkn_type type) { return &parse_rules[type]; }

static int parse_variable(b_parser *p, const char *message) {
  consume(p, IDENTIFIER_TOKEN, message);

  declare_variable(p);
  if (p->compiler->scope_depth > 0) // we are in a local scope...
    return 0;

  return identifier_constant(p, &p->previous);
}

static void mark_initalized(b_parser *p) {
  p->compiler->locals[p->compiler->local_count - 1].depth =
      p->compiler->scope_depth;
}

static void define_variable(b_parser *p, int global) {
  if (p->compiler->scope_depth > 0) { // we are in a local scope...
    mark_initalized(p);
    return;
  }

  if (global <= UINT8_MAX) { // constant
    emit_bytes(p, OP_DEFINE_GLOBAL, global);
  } else { // long constant
    emit_bytes(p, OP_DEFINE_LGLOBAL, global);
  }
}

static void expression(b_parser *p) { parse_precedence(p, PREC_ASSIGNMENT); }

static void block(b_parser *p) {
  ignore_whitespace(p);
  while (!check(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    declaration(p);
  }
  consume(p, RBRACE_TOKEN, "expected '}' after block");
}

static void var_declaration(b_parser *p) {
  int global = parse_variable(p, "variable name expected");

  if (match(p, EQUAL_TOKEN)) {
    expression(p);
  } else {
    emit_byte(p, OP_NIL);
  }

  consume_statement_end(p);
  define_variable(p, global);
}

static void expression_statement(b_parser *p) {
  expression(p);
  consume_statement_end(p);
  emit_byte(p, OP_POP);
}

static void echo_statement(b_parser *p) {
  expression(p);
  consume_statement_end(p);
  emit_byte(p, OP_ECHO);
}

static void synchronize(b_parser *p) {
  p->panic_mode = false;

  while (p->current.type != EOF_TOKEN) {
    if (p->current.type == NEWLINE_TOKEN || p->current.type == SEMICOLON_TOKEN)
      return;

    switch (p->current.type) {
    case CLASS_TOKEN:
    case DEF_TOKEN:
    case VAR_TOKEN:
    case FOR_TOKEN:
    case IF_TOKEN:
    case USING_TOKEN:
    case ITER_TOKEN:
    case WHILE_TOKEN:
    case ECHO_TOKEN:
    case DIE_TOKEN:
    case RETURN_TOKEN:
    case STATIC_TOKEN:
      return;

    default:; // do nothing
    }

    advance(p);
  }
}

static void declaration(b_parser *p) {
  ignore_whitespace(p);

  if (match(p, VAR_TOKEN)) {
    var_declaration(p);
  } else {
    statement(p);
  }

  ignore_whitespace(p);

  if (p->panic_mode)
    synchronize(p);
}

static void statement(b_parser *p) {
  ignore_whitespace(p);

  if (match(p, ECHO_TOKEN)) {
    echo_statement(p);
  } else if (match(p, LBRACE_TOKEN)) {
    begin_scope(p);
    block(p);
    end_scope(p);
  } else {
    expression_statement(p);
  }

  ignore_whitespace(p);
}

bool compile(b_vm *vm, const char *source, b_blob *blob) {
  b_scanner scanner;
  init_scanner(&scanner, source);

  b_parser parser;

  parser.vm = vm;
  parser.scanner = &scanner;

  parser.had_error = false;
  parser.panic_mode = false;
  parser.current_blob = blob;

  b_compiler compiler;
  init_compiler(&parser, &compiler);

  advance(&parser);

  while (!match(&parser, EOF_TOKEN)) {
    declaration(&parser);
  }

  end_compiler(&parser);
  return !parser.had_error;
}