#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "config.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "util.h"

#if DEBUG_MODE == 1
#include "debug.h"
#endif

static b_blob *current_blob(b_parser *p) {
  return &p->compiler->function->blob;
}

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

  // allow block last statement to ommit statement end
  if (p->in_block && check(p, RBRACE_TOKEN))
    return;

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

static int get_code_args_count(const uint8_t *bytecode,
                               const b_value *constants, int ip) {
  b_code code = (b_code)bytecode[ip];

  // @TODO: handle upvalues gracefully...
  switch (code) {
  case OP_EQUAL:
  case OP_GREATER:
  case OP_LESS:
  case OP_NIL:
  case OP_TRUE:
  case OP_FALSE:
  case OP_ADD:
  case OP_SUBTRACT:
  case OP_MULTIPLY:
  case OP_DIVIDE:
  case OP_FDIVIDE:
  case OP_REMINDER:
  case OP_POW:
  case OP_NEGATE:
  case OP_NOT:
  case OP_ECHO:
  case OP_POP:
  case OP_CLOSE_UPVALUE:
  case OP_DUP:
  case OP_RETURN:
    return 0;

  case OP_CALL:
    return 1;

  case OP_DEFINE_GLOBAL:
  case OP_GET_GLOBAL:
  case OP_SET_GLOBAL:
  case OP_GET_LOCAL:
  case OP_SET_LOCAL:
  case OP_GET_UPVALUE:
  case OP_SET_UPVALUE:
  case OP_JUMP_IF_FALSE:
  case OP_JUMP:
  case OP_BREAK_PL:
  case OP_LOOP:
  case OP_CONSTANT:
  case OP_POPN:
  case OP_CLASS:
  case OP_GET_PROPERTY:
  case OP_SET_PROPERTY:
  case OP_CLASS_PROPERTY:
  case OP_METHOD:
    return 2;

  case OP_CLOSURE: {
    int constant = (bytecode[ip + 1] << 8) | bytecode[ip + 2];
    b_obj_func *fn = AS_FUNCTION(constants[constant]);

    // There is two byte for the constant, then three for each upvalue.
    // @TODO: change 2 to 3 when supporting variadic index...
    return 2 + (fn->upvalue_count * 3);
  }
  }
  return 0;
}

static void emit_byte(b_parser *p, uint8_t byte) {
  write_blob(p->vm, current_blob(p), byte, p->previous.line);
}

static void emit_short(b_parser *p, uint16_t byte) {
  write_blob(p->vm, current_blob(p), (byte >> 8) & 0xff, p->previous.line);
  write_blob(p->vm, current_blob(p), byte & 0xff, p->previous.line);
}

static void emit_bytes(b_parser *p, uint8_t byte, uint8_t byte2) {
  write_blob(p->vm, current_blob(p), byte, p->previous.line);
  write_blob(p->vm, current_blob(p), byte2, p->previous.line);
}

static void emit_byte_and_short(b_parser *p, uint8_t byte, uint16_t byte2) {
  write_blob(p->vm, current_blob(p), byte, p->previous.line);
  write_blob(p->vm, current_blob(p), (byte2 >> 8) & 0xff, p->previous.line);
  write_blob(p->vm, current_blob(p), byte2 & 0xff, p->previous.line);
}

/* static void emit_byte_and_long(b_parser *p, uint8_t byte, uint16_t byte2) {
  write_blob(p->vm, current_blob(p), byte, p->previous.line);
  write_blob(p->vm, current_blob(p), (byte2 >> 16) & 0xff, p->previous.line);
  write_blob(p->vm, current_blob(p), (byte2 >> 8) & 0xff, p->previous.line);
  write_blob(p->vm, current_blob(p), byte2 & 0xff, p->previous.line);
} */

static void emit_loop(b_parser *p, int loop_start) {
  emit_byte(p, OP_LOOP);

  int offset = current_blob(p)->count - loop_start + 2;
  if (offset > UINT16_MAX)
    error(p, "loop body too large");

  emit_byte(p, (offset >> 8) & 0xff);
  emit_byte(p, offset & 0xff);
}

static void emit_return(b_parser *p) {
  emit_byte(p, OP_NIL);
  emit_byte(p, OP_RETURN);
}

static int make_constant(b_parser *p, b_value value) {
  int constant = add_constant(p->vm, current_blob(p), value);
  if (constant >= UINT16_MAX) {
    error(p, "too many constants in current scope");
    return 0;
  }
  return constant;
}

static void emit_constant(b_parser *p, b_value value) {
  int constant = make_constant(p, value);
  emit_byte_and_short(p, OP_CONSTANT, (uint16_t)constant);
}

static int emit_jump(b_parser *p, uint8_t instruction) {
  emit_byte(p, instruction);

  // placeholders
  emit_byte(p, 0xff);
  emit_byte(p, 0xff);

  return current_blob(p)->count - 2;
}

static void patch_jump(b_parser *p, int offset) {
  // -2 to adjust the bytecode for the offset itself
  int jump = current_blob(p)->count - offset - 2;

  if (jump > UINT16_MAX) {
    error(p, "body of conditional block too large");
  }

  current_blob(p)->code[offset] = (jump >> 8) & 0xff;
  current_blob(p)->code[offset + 1] = jump & 0xff;
}

static void init_compiler(b_parser *p, b_compiler *compiler, b_func_type type) {
  compiler->enclosing = p->compiler;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;

  compiler->function = new_function(p->vm);
  p->compiler = compiler;

  if (type != TYPE_SCRIPT) {
    p->compiler->function->name =
        copy_string(p->vm, p->previous.start, p->previous.length);
  }

  // claiming slot zero for use in class methods
  b_local *local = &p->compiler->locals[p->compiler->local_count++];
  local->depth = 0;
  local->is_captured = false;
  local->name.start = "";
  local->name.length = 0;
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

static int resolve_local(b_parser *p, b_compiler *compiler, b_token *name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    b_local *local = &compiler->locals[i];
    if (identifiers_equal(&local->name, name)) {
      if (local->depth == -1) {
        error(p, "cannot read local variable in it's own initializer");
      }
      return i;
    }
  }
  return -1;
}

static int add_upvalue(b_parser *p, b_compiler *compiler, uint16_t index,
                       bool is_local) {
  int upvalue_count = compiler->function->upvalue_count;

  for (int i = 0; i < upvalue_count; i++) {
    b_upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }

  if (upvalue_count == UINT8_COUNT) {
    error(p, "too many closure variables in function");
    return 0;
  }

  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->function->upvalue_count++;
}

static int resolve_upvalue(b_parser *p, b_compiler *compiler, b_token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolve_local(p, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].is_captured = true;
    return add_upvalue(p, compiler, (uint16_t)local, true);
  }

  int upvalue = resolve_upvalue(p, compiler->enclosing, name);
  if (upvalue != -1) {
    return add_upvalue(p, compiler, (uint16_t)upvalue, false);
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
  local->is_captured = false;
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
static int parse_variable(b_parser *p, const char *message) {
  consume(p, IDENTIFIER_TOKEN, message);

  declare_variable(p);
  if (p->compiler->scope_depth > 0) // we are in a local scope...
    return 0;

  return identifier_constant(p, &p->previous);
}

static void mark_initalized(b_parser *p) {
  if (p->compiler->scope_depth == 0)
    return;

  p->compiler->locals[p->compiler->local_count - 1].depth =
      p->compiler->scope_depth;
}

static void define_variable(b_parser *p, int global) {
  if (p->compiler->scope_depth > 0) { // we are in a local scope...
    mark_initalized(p);
    return;
  }

  emit_byte_and_short(p, OP_DEFINE_GLOBAL, global);
}

static b_obj_func *end_compiler(b_parser *p) {
  emit_return(p);
  b_obj_func *function = p->compiler->function;

#ifdef DEBUG_PRINT_CODE
#if DEBUG_PRINT_CODE == 1
  if (!p->had_error) {
    disassemble_blob(current_blob(p), function->name == NULL
                                          ? "<script>"
                                          : function->name->chars);
  }
#endif
#endif
  p->compiler = p->compiler->enclosing;
  return function;
}

static void begin_scope(b_parser *p) { p->compiler->scope_depth++; }

static void end_scope(b_parser *p) {
  p->compiler->scope_depth--;

  // remove all variables declared in scope while exiting...
  while (p->compiler->local_count > 0 &&
         p->compiler->locals[p->compiler->local_count - 1].depth >
             p->compiler->scope_depth) {
    if (p->compiler->locals[p->compiler->local_count - 1].is_captured) {
      emit_byte(p, OP_CLOSE_UPVALUE);
    } else {
      emit_byte(p, OP_POP);
    }
    p->compiler->local_count--;
  }
}

static void discard_local(b_parser *p, int depth) {
  if (p->compiler->scope_depth == -1) {
    error(p, "cannot exit top-level scope");
  }
  for (int i = p->compiler->local_count;
       i >= 0 && p->compiler->locals[i].depth > depth; i--) {
    if (p->compiler->locals[i].is_captured) {
      emit_byte(p, OP_CLOSE_UPVALUE);
    } else {
      emit_byte(p, OP_POP);
    }
  }
}

static void end_loop(b_parser *p) {
  // find all OP_BREAK_PL placeholder and replace with the appropriate jump...
  int i = p->innermost_loop_start;

  while (i < p->compiler->function->blob.count) {
    if (p->compiler->function->blob.code[i] == OP_BREAK_PL) {
      p->compiler->function->blob.code[i] = OP_JUMP;
      patch_jump(p, i + 1);
    } else {
      i += 1 + get_code_args_count(p->compiler->function->blob.code,
                                   p->compiler->function->blob.constants.values,
                                   i);
    }
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

static uint8_t argument_list(b_parser *p) {
  uint8_t arg_count = 0;
  if (!check(p, RPAREN_TOKEN)) {
    do {
      ignore_whitespace(p);
      expression(p);
      if (arg_count == MAX_FUNCTION_PARAMETERS) {
        error(p, "cannot have more than %d arguments to a function",
              MAX_FUNCTION_PARAMETERS);
      }
      arg_count++;
    } while (match(p, COMMA_TOKEN));
  }
  ignore_whitespace(p);
  consume(p, RPAREN_TOKEN, "expected ')' after argument list");
  return arg_count;
}

static void call(b_parser *p, bool can_assign) {
  uint8_t arg_count = argument_list(p);
  emit_bytes(p, OP_CALL, arg_count);
}

static void dot(b_parser *p, bool can_assign) {
  consume(p, IDENTIFIER_TOKEN, "expected property name after '.'");
  int name = identifier_constant(p, &p->previous);

  if (can_assign && match(p, EQUAL_TOKEN)) {
    expression(p);
    emit_byte_and_short(p, OP_SET_PROPERTY, name);
  } else {
    emit_byte_and_short(p, OP_GET_PROPERTY, name);
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
  int arg = resolve_local(p, p->compiler, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if ((arg = resolve_upvalue(p, p->compiler, &name)) != -1) {
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  } else {
    arg = identifier_constant(p, &name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(p, EQUAL_TOKEN)) {
    expression(p);
    emit_byte_and_short(p, set_op, (uint16_t)arg);
  } else {
    emit_byte_and_short(p, get_op, (uint16_t)arg);
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

static void and_(b_parser *p, bool can_assign) {
  int end_jump = emit_jump(p, OP_JUMP_IF_FALSE);

  emit_byte(p, OP_POP);
  parse_precedence(p, PREC_AND);

  patch_jump(p, end_jump);
}

static void or_(b_parser *p, bool can_assign) {
  int else_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(p, OP_JUMP);

  patch_jump(p, else_jump);
  emit_byte(p, OP_POP);

  parse_precedence(p, PREC_OR);
  patch_jump(p, end_jump);
}

b_parse_rule parse_rules[] = {
    // symbols
    [NEWLINE_TOKEN] = {NULL, NULL, PREC_NONE},            // (
    [LPAREN_TOKEN] = {grouping, call, PREC_CALL},         // (
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
    [DOT_TOKEN] = {NULL, dot, PREC_CALL},                 // .
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
    [AND_TOKEN] = {NULL, and_, PREC_AND},
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
    [OR_TOKEN] = {NULL, or_, PREC_OR},
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

static void expression(b_parser *p) { parse_precedence(p, PREC_ASSIGNMENT); }

static void block(b_parser *p) {
  p->in_block = true;
  ignore_whitespace(p);
  while (!check(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    declaration(p);
  }
  p->in_block = false;
  consume(p, RBRACE_TOKEN, "expected '}' after block");
}

static void function(b_parser *p, b_func_type type) {
  b_compiler compiler;
  init_compiler(p, &compiler, type);
  begin_scope(p);

  // compile parameter list
  consume(p, LPAREN_TOKEN, "expected '(' after function name");
  if (!check(p, RPAREN_TOKEN)) {
    // compile argument list...
    do {
      p->compiler->function->arity++;
      if (p->compiler->function->arity > MAX_FUNCTION_PARAMETERS) {
        error_at_current(p, "cannot have more than %d function parameters",
                         MAX_FUNCTION_PARAMETERS);
      }

      int param_constant = parse_variable(p, "expected parameter name");
      define_variable(p, param_constant);
    } while (match(p, COMMA_TOKEN));
  }
  consume(p, RPAREN_TOKEN, "expected ')' after function parameters");

  // compile the body
  consume(p, LBRACE_TOKEN, "expected '{' before function body");
  block(p);

  // create the function object
  b_obj_func *function = end_compiler(p);
  emit_byte_and_short(p, OP_CLOSURE, make_constant(p, OBJ_VAL(function)));

  for (int i = 0; i < function->upvalue_count; i++) {
    emit_byte(p, compiler.upvalues[i].is_local ? 1 : 0);
    emit_short(p, compiler.upvalues[i].index);
  }
}

static void method(b_parser *p) {
  consume(p, IDENTIFIER_TOKEN, "method name expected");
  int constant = identifier_constant(p, &p->previous);

  b_func_type type = TYPE_FUNCTION;
  function(p, type);
  emit_byte_and_short(p, OP_METHOD, constant);
}

static void function_declaration(b_parser *p) {
  int global = parse_variable(p, "function name expected");
  mark_initalized(p);
  function(p, TYPE_FUNCTION);
  define_variable(p, global);
}

static void class_declaration(b_parser *p) {
  consume(p, IDENTIFIER_TOKEN, "class name expected");
  int name_constant = identifier_constant(p, &p->previous);
  b_token class_name = p->previous;
  declare_variable(p);

  emit_byte_and_short(p, OP_CLASS, name_constant);
  define_variable(p, name_constant);

  named_variable(p, class_name, false);

  consume(p, LBRACE_TOKEN, "expected '{' before class body");
  ignore_whitespace(p);
  while (!check(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    if (match(p, VAR_TOKEN)) {
      consume(p, IDENTIFIER_TOKEN, "method name expected");
      int field_constant = identifier_constant(p, &p->previous);

      consume(p, EQUAL_TOKEN, "expected '=' after class field name");
      expression(p);
      consume_statement_end(p);
      ignore_whitespace(p);

      emit_byte_and_short(p, OP_CLASS_PROPERTY, field_constant);
    } else {
      method(p);
      ignore_whitespace(p);
    }
  }
  consume(p, RBRACE_TOKEN, "expected '}' after class body");
  emit_byte(p, OP_POP);
}

static void _var_declaration(b_parser *p, bool is_initalizer) {
  int global = parse_variable(p, "variable name expected");

  if (match(p, EQUAL_TOKEN)) {
    expression(p);
  } else {
    emit_byte(p, OP_NIL);
  }

  if (!is_initalizer) {
    consume_statement_end(p);
  } else {
    consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
  }

  define_variable(p, global);
}

static void var_declaration(b_parser *p) { _var_declaration(p, false); }

static void _expression_statement(b_parser *p, bool is_initalizer) {
  expression(p);
  if (!is_initalizer) {
    consume_statement_end(p);
  } else {
    consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
  }
  emit_byte(p, OP_POP);
}

static void expression_statement(b_parser *p) {
  _expression_statement(p, false);
}

/**
 * iter statements are like for loops in c...
 * they are desugared into a while loop
 *
 * i.e.
 *
 * iter i = 0; i < 10; i++ {
 *    ...
 * }
 *
 * desugars into:
 *
 * var i = 0
 * while i < 10 {
 *    ...
 *    i = i + 1
 * }
 */
static void iter_statement(b_parser *p) {
  begin_scope(p);

  // parse intializer...
  if (match(p, SEMICOLON_TOKEN)) {
    // no intializer
  } else if (match(p, VAR_TOKEN)) {
    _var_declaration(p, true);
  } else {
    _expression_statement(p, true);
  }

  // keep a copy of the surrounding loop's start and depth
  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // update the parser's loop start and depth to the current
  p->innermost_loop_start = current_blob(p)->count;
  p->innermost_loop_scope_depth = p->compiler->scope_depth;

  int exit_jump = -1;
  if (!match(p, SEMICOLON_TOKEN)) { // the condition is optional
    expression(p);
    consume(p, SEMICOLON_TOKEN, "expected ';' after condition");

    // jump out of the loop if the condition is false...
    exit_jump = emit_jump(p, OP_JUMP_IF_FALSE);
    emit_byte(p, OP_POP); // pop the condition
  }

  // the iterator...
  if (!match(p, LBRACE_TOKEN)) {
    int body_jump = emit_jump(p, OP_JUMP);

    int increment_start = current_blob(p)->count;
    expression(p);
    emit_byte(p, OP_POP);

    emit_loop(p, p->innermost_loop_start);
    p->innermost_loop_start = increment_start;
    patch_jump(p, body_jump);
  }

  statement(p);

  emit_loop(p, p->innermost_loop_start);

  if (exit_jump != -1) {
    patch_jump(p, exit_jump);
    emit_byte(p, OP_POP);
  }

  end_loop(p);

  // reset the loop start and scope depth to the surrounding value
  p->innermost_loop_start = surrounding_loop_start;
  p->innermost_loop_scope_depth = surrounding_scope_depth;

  end_scope(p);
}

/**
 * using expression {
 *    when expression {
 *      ...
 *    }
 *    when expression {
 *      ...
 *    }
 *    ...
 * }
 */
static void using_statement(b_parser *p) {
  expression(p); // the expression
  consume(p, LBRACE_TOKEN, "expected '{' after using expression");
  ignore_whitespace(p);

  int state = 0; // 0: before all cases, 1: before default, 2: after default
  int case_ends[MAX_USING_CASES];
  int case_count = 0;
  int previous_case_skip = -1;

  while (!match(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    if (match(p, WHEN_TOKEN) || match(p, DEFAULT_TOKEN)) {
      b_tkn_type case_type = p->previous.type;

      if (state == 2) {
        error(p, "cannot have another case after a default case");
      }

      if (state == 1) {
        // at the end of the previous case, jump over the others...
        case_ends[case_count++] = emit_jump(p, OP_JUMP);

        // patch it's condition to jump to the next case (this one)
        patch_jump(p, previous_case_skip);
        emit_byte(p, OP_POP);
      }

      if (case_type == WHEN_TOKEN) {
        state = 1;

        // check if the case is equal to the value...
        emit_byte(p, OP_DUP);
        expression(p);

        emit_byte(p, OP_EQUAL);
        previous_case_skip = emit_jump(p, OP_JUMP_IF_FALSE);

        // pop the result of the comparison
        emit_byte(p, OP_POP);
      } else {
        state = 2;
        previous_case_skip = -1;
      }
    } else {
      // otherwise, it's a statement inside the current case
      if (state == 0) {
        error(p, "cannot have statements before any case");
      }
      statement(p);
    }
  }

  // if we ended without a default case, patch its condition jump
  if (state == 1) {
    patch_jump(p, previous_case_skip);
    emit_byte(p, OP_POP);
  }

  // patch all the case jumps to the end
  for (int i = 0; i < case_count; i++) {
    patch_jump(p, case_ends[i]);
  }

  emit_byte(p, OP_POP);
}

static void if_statement(b_parser *p) {
  expression(p);

  int then_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);
  statement(p);

  int else_jump = emit_jump(p, OP_JUMP);

  patch_jump(p, then_jump);
  emit_byte(p, OP_POP);

  if (match(p, ELSE_TOKEN)) {
    statement(p);
  }

  patch_jump(p, else_jump);
}

static void echo_statement(b_parser *p) {
  expression(p);
  consume_statement_end(p);
  emit_byte(p, OP_ECHO);
}

static void return_statement(b_parser *p) {
  if (p->compiler->type == TYPE_SCRIPT) {
    error(p, "cannot return from top-level code");
  }

  if (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN)) {
    emit_return(p);
  } else {
    expression(p);
    consume_statement_end(p);
    emit_byte(p, OP_RETURN);
  }
}

static void while_statement(b_parser *p) {
  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // we'll be jumping back to right before the
  // expression after the loop body
  p->innermost_loop_start = current_blob(p)->count;

  expression(p);

  int exit_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);

  statement(p);

  emit_loop(p, p->innermost_loop_start);

  patch_jump(p, exit_jump);
  emit_byte(p, OP_POP);

  end_loop(p);

  p->innermost_loop_start = surrounding_loop_start;
  p->innermost_loop_scope_depth = surrounding_scope_depth;
}

static void continue_statement(b_parser *p) {
  if (p->innermost_loop_start == -1) {
    error(p, "'continue' can only be used in a loop");
  }
  consume_statement_end(p);

  // discard local variables created in the loop
  discard_local(p, p->innermost_loop_scope_depth);

  // go back to the top of the loop
  emit_loop(p, p->innermost_loop_start);
}

static void break_statement(b_parser *p) {
  if (p->innermost_loop_start == -1) {
    error(p, "'break' can only be used in a loop");
  }
  consume_statement_end(p);

  // discard local variables created in the loop
  discard_local(p, p->innermost_loop_scope_depth);
  emit_jump(p, OP_BREAK_PL);
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

  if (match(p, CLASS_TOKEN)) {
    class_declaration(p);
  } else if (match(p, DEF_TOKEN)) {
    function_declaration(p);
  } else if (match(p, VAR_TOKEN)) {
    var_declaration(p);
  } else {
    statement(p);
  }

  ignore_whitespace(p);

  if (p->panic_mode)
    synchronize(p);

  ignore_whitespace(p);
}

static void statement(b_parser *p) {
  ignore_whitespace(p);

  if (match(p, ECHO_TOKEN)) {
    echo_statement(p);
  } else if (match(p, IF_TOKEN)) {
    if_statement(p);
  } else if (match(p, WHILE_TOKEN)) {
    while_statement(p);
  } else if (match(p, ITER_TOKEN)) {
    iter_statement(p);
  } else if (match(p, USING_TOKEN)) {
    using_statement(p);
  } else if (match(p, CONTINUE_TOKEN)) {
    continue_statement(p);
  } else if (match(p, BREAK_TOKEN)) {
    break_statement(p);
  } else if (match(p, RETURN_TOKEN)) {
    return_statement(p);
  } else if (match(p, LBRACE_TOKEN)) {
    begin_scope(p);
    block(p);
    end_scope(p);
  } else {
    expression_statement(p);
  }

  ignore_whitespace(p);
}

b_obj_func *compile(b_vm *vm, const char *source, b_blob *blob) {
  b_scanner scanner;
  init_scanner(&scanner, source);

  b_parser parser;

  parser.vm = vm;
  parser.scanner = &scanner;

  parser.had_error = false;
  parser.panic_mode = false;
  parser.in_block = false;
  parser.innermost_loop_start = -1;
  parser.innermost_loop_scope_depth = 0;
  parser.compiler = NULL;

  b_compiler compiler;
  init_compiler(&parser, &compiler, TYPE_SCRIPT);

  advance(&parser);

  while (!match(&parser, EOF_TOKEN)) {
    declaration(&parser);
  }

  b_obj_func *function = end_compiler(&parser);

  vm->compiler = &compiler;

  return parser.had_error ? NULL : function;
}

void mark_compiler_roots(b_vm *vm) {
  b_compiler *compiler = vm->compiler;
  while (compiler != NULL) {
    mark_object(vm, (b_obj *)compiler->function);
    compiler = compiler->enclosing;
  }
}