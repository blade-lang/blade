#include "compiler.h"
#include "common.h"
#include "config.h"
#include "memory.h"
#include "object.h"
#include "pathinfo.h"
#include "scanner.h"
#include "utf8.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

static b_blob *current_blob(b_parser *p) {
  return &p->vm->compiler->function->blob;
}

static void error_at(b_parser *p, b_token *t, const char *message,
                     va_list args) {
  fflush(stdout); // flush out anything on stdout first

  // do not cascade error
  // suppress error if already in panic mode
  if (p->panic_mode)
    return;

  p->panic_mode = true;

  fprintf(stderr, "SyntaxError");

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
  fprintf(stderr, "  %s:%d\n", p->module->file, t->line);

  p->had_error = true;
}

static void error(b_parser *p, const char *message, ...) {
  va_list args;
  va_start(args, message);
  error_at(p, &p->previous, message, args);
  va_end(args);
}

static void error_at_current(b_parser *p, const char *message, ...) {
  va_list args;
  va_start(args, message);
  error_at(p, &p->current, message, args);
  va_end(args);
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

static void consume_or(b_parser *p, const char *message, const b_tkn_type ts[], int count) {

  for (int i = 0; i < count; i++) {
    if (p->current.type == ts[i]) {
      advance(p);
      return;
    }
  }

  error_at_current(p, message);
}

static bool check_number(b_parser *p) {
  if (p->previous.type == REG_NUMBER_TOKEN ||
      p->previous.type == OCT_NUMBER_TOKEN ||
      p->previous.type == BIN_NUMBER_TOKEN ||
      p->previous.type == HEX_NUMBER_TOKEN)
    return true;
  return false;
}

static bool check(b_parser *p, b_tkn_type t) { return p->current.type == t; }

static bool match(b_parser *p, b_tkn_type t) {
  if (!check(p, t))
    return false;
  advance(p);
  return true;
}

static void consume_statement_end(b_parser *p) {

  // allow block last statement to omit statement end
  if (p->block_count > 0 && check(p, RBRACE_TOKEN))
    return;

  if (match(p, SEMICOLON_TOKEN)) {
    while (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN));
    return;
  }

  if (match(p, EOF_TOKEN) || p->previous.type == EOF_TOKEN)
    return;

  consume(p, NEWLINE_TOKEN, "end of statement expected");
  while (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN));
}

static void ignore_whitespace(b_parser *p) {
  while (match(p, NEWLINE_TOKEN));
}

static int get_code_args_count(const uint8_t *bytecode,
                               const b_value *constants, int ip) {
  b_code code = (b_code) bytecode[ip];

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
    case OP_F_DIVIDE:
    case OP_REMINDER:
    case OP_POW:
    case OP_NEGATE:
    case OP_NOT:
    case OP_ECHO:
    case OP_POP:
    case OP_CLOSE_UP_VALUE:
    case OP_DUP:
    case OP_RETURN:
    case OP_INHERIT:
    case OP_GET_SUPER:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_LSHIFT:
    case OP_RSHIFT:
    case OP_BIT_NOT:
    case OP_ONE:
    case OP_SET_INDEX:
    case OP_ASSERT:
    case OP_RANGE:
    case OP_STRINGIFY:
    case OP_CHOICE:
    case OP_EMPTY:
    case OP_DIE:
    case OP_IMPORT_ALL_NATIVE:
    case OP_IMPORT_ALL:
    case OP_END_CATCH:
      return 0;

    case OP_CALL:
    case OP_SUPER_INVOKE_SELF:
    case OP_GET_INDEX:
    case OP_GET_RANGED_INDEX:
      return 1;

    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
    case OP_GET_LOCAL:
    case OP_SET_LOCAL:
    case OP_GET_UP_VALUE:
    case OP_SET_UP_VALUE:
    case OP_JUMP_IF_FALSE:
    case OP_JUMP:
    case OP_BREAK_PL:
    case OP_LOOP:
    case OP_CONSTANT:
    case OP_POP_N:
    case OP_CLASS:
    case OP_GET_PROPERTY:
    case OP_GET_SELF_PROPERTY:
    case OP_SET_PROPERTY:
    case OP_LIST:
    case OP_DICT:
    case OP_CALL_IMPORT:
    case OP_NATIVE_MODULE:
    case OP_SELECT_NATIVE_IMPORT:
    case OP_SWITCH:
    case OP_METHOD:
    case OP_EJECT_IMPORT:
    case OP_EJECT_NATIVE_IMPORT:
    case OP_SELECT_IMPORT:
    case OP_BEGIN_CATCH:
      return 2;

    case OP_INVOKE:
    case OP_INVOKE_SELF:
    case OP_SUPER_INVOKE:
    case OP_CLASS_PROPERTY:
      return 3;

    case OP_CLOSURE: {
      int constant = (bytecode[ip + 1] << 8) | bytecode[ip + 2];
      b_obj_func *fn = AS_FUNCTION(constants[constant]);

      // There is two byte for the constant, then three for each up value.
      return 2 + (fn->up_value_count * 3);
    }

    default:
      return 0;
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
  if (p->vm->compiler->type == TYPE_INITIALIZER) {
    emit_byte_and_short(p, OP_GET_LOCAL, 0);
  } else {
    emit_byte(p, OP_EMPTY);
  }
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
  emit_byte_and_short(p, OP_CONSTANT, (uint16_t) constant);
}

static int emit_jump(b_parser *p, uint8_t instruction) {
  emit_byte(p, instruction);

  // placeholders
  emit_byte(p, 0xff);
  emit_byte(p, 0xff);

  return current_blob(p)->count - 2;
}

static int emit_switch(b_parser *p) {
  emit_byte(p, OP_SWITCH);

  // placeholders
  emit_byte(p, 0xff);
  emit_byte(p, 0xff);

  return current_blob(p)->count - 2;
}

static void patch_with_value(b_parser *p, int offset, int constant) {
  current_blob(p)->code[offset] = (constant >> 8) & 0xff;
  current_blob(p)->code[offset + 1] = constant & 0xff;
}

static void patch_try(b_parser *p, int offset, int type, int address, int finally) {
  // patch type
  current_blob(p)->code[offset] = (type >> 8) & 0xff;
  current_blob(p)->code[offset + 1] = type & 0xff;
  // patch address
  current_blob(p)->code[offset + 2] = (address >> 8) & 0xff;
  current_blob(p)->code[offset + 3] = address & 0xff;
  // patch finally
  current_blob(p)->code[offset + 4] = (finally >> 8) & 0xff;
  current_blob(p)->code[offset + 5] = finally & 0xff;
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
  compiler->enclosing = p->vm->compiler;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->handler_count = 0;

  compiler->function = new_function(p->vm, p->module, type);
  p->vm->compiler = compiler;

  if (type != TYPE_SCRIPT) {
    push(p->vm, OBJ_VAL(compiler->function));
    p->vm->compiler->function->name =
        copy_string(p->vm, p->previous.start, p->previous.length);
    pop(p->vm);
  }

  // claiming slot zero for use in class methods
  b_local *local = &p->vm->compiler->locals[p->vm->compiler->local_count++];
  local->depth = 0;
  local->is_captured = false;

  if (type != TYPE_FUNCTION) {
    local->name.start = "self";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static int identifier_constant(b_parser *p, b_token *name) {
  return make_constant(p,
                       OBJ_VAL(copy_string(p->vm, name->start, name->length)));
}

static inline bool identifiers_equal(b_token *a, b_token *b) {
  return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
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

static int add_up_value(b_parser *p, b_compiler *compiler, uint16_t index,
                        bool is_local) {
  int up_value_count = compiler->function->up_value_count;

  for (int i = 0; i < up_value_count; i++) {
    b_up_value *up_value = &compiler->up_values[i];
    if (up_value->index == index && up_value->is_local == is_local) {
      return i;
    }
  }

  if (up_value_count == UINT8_COUNT) {
    error(p, "too many closure variables in function");
    return 0;
  }

  compiler->up_values[up_value_count].is_local = is_local;
  compiler->up_values[up_value_count].index = index;
  return compiler->function->up_value_count++;
}

static int resolve_up_value(b_parser *p, b_compiler *compiler, b_token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolve_local(p, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].is_captured = true;
    return add_up_value(p, compiler, (uint16_t) local, true);
  }

  int up_value = resolve_up_value(p, compiler->enclosing, name);
  if (up_value != -1) {
    return add_up_value(p, compiler, (uint16_t) up_value, false);
  }

  return -1;
}

static int add_local(b_parser *p, b_token name) {
  if (p->vm->compiler->local_count == UINT8_COUNT) {
    // we've reached maximum local variables per scope
    error(p, "too many local variables in scope");
    return -1;
  }

  b_local *local = &p->vm->compiler->locals[p->vm->compiler->local_count++];
  local->name = name;
  local->depth = -1;
  local->is_captured = false;
  return p->vm->compiler->local_count;
}

static void declare_variable(b_parser *p) {
  // global variables are implicitly declared...
  if (p->vm->compiler->scope_depth == 0)
    return;

  b_token *name = &p->previous;

  for (int i = p->vm->compiler->local_count - 1; i >= 0; i--) {
    b_local *local = &p->vm->compiler->locals[i];
    if (local->depth != -1 && local->depth < p->vm->compiler->scope_depth) {
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
  if (p->vm->compiler->scope_depth > 0) // we are in a local scope...
    return 0;

  return identifier_constant(p, &p->previous);
}

static void mark_initialized(b_parser *p) {
  if (p->vm->compiler->scope_depth == 0)
    return;

  p->vm->compiler->locals[p->vm->compiler->local_count - 1].depth =
      p->vm->compiler->scope_depth;
}

static void define_variable(b_parser *p, int global) {
  if (p->vm->compiler->scope_depth > 0) { // we are in a local scope...
    mark_initialized(p);
    return;
  }

  emit_byte_and_short(p, OP_DEFINE_GLOBAL, global);
}

static b_token synthetic_token(const char *name) {
  b_token token;
  token.start = name;
  token.length = (int) strlen(name);
  return token;
}

static b_obj_func *end_compiler(b_parser *p) {
  emit_return(p);
  b_obj_func *function = p->vm->compiler->function;

  if (!p->had_error && p->vm->should_print_bytecode) {
    disassemble_blob(current_blob(p), function->name == NULL
                                      ? p->module->file
                                      : function->name->chars);
  }

  p->vm->compiler = p->vm->compiler->enclosing;
  return function;
}

static void begin_scope(b_parser *p) { p->vm->compiler->scope_depth++; }

static void end_scope(b_parser *p) {
  p->vm->compiler->scope_depth--;

  // remove all variables declared in scope while exiting...
  while (p->vm->compiler->local_count > 0 &&
         p->vm->compiler->locals[p->vm->compiler->local_count - 1].depth >
         p->vm->compiler->scope_depth) {
    if (p->vm->compiler->locals[p->vm->compiler->local_count - 1].is_captured) {
      emit_byte(p, OP_CLOSE_UP_VALUE);
    } else {
      emit_byte(p, OP_POP);
    }
    p->vm->compiler->local_count--;
  }
}

static int discard_locals(b_parser *p, int depth) {
  if (p->vm->compiler->scope_depth == -1) {
    error(p, "cannot exit top-level scope");
  }

  int local = p->vm->compiler->local_count - 1;
  while (local >= 0 && p->vm->compiler->locals[local].depth >= depth) {
    if (p->vm->compiler->locals[local].is_captured) {
      emit_byte(p, OP_CLOSE_UP_VALUE);
    } else {
      emit_byte(p, OP_POP);
    }
    local--;
  }

  return p->vm->compiler->local_count - local - 1;
}

static void end_loop(b_parser *p) {
  // find all OP_BREAK_PL placeholder and replace with the appropriate jump...
  int i = p->innermost_loop_start;

  while (i < p->vm->compiler->function->blob.count) {
    if (p->vm->compiler->function->blob.code[i] == OP_BREAK_PL) {
      p->vm->compiler->function->blob.code[i] = OP_JUMP;
      patch_jump(p, i + 1);
      i += 3;
    } else {
      i += 1 + get_code_args_count(p->vm->compiler->function->blob.code,
                                   p->vm->compiler->function->blob.constants.values,
                                   i);
    }
  }
}

// --> Forward declarations start
static void expression(b_parser *p);

static void statement(b_parser *p);

static void declaration(b_parser *p);

static void anonymous(b_parser *p, bool can_assign);

static b_parse_rule *get_rule(b_tkn_type type);

static void parse_precedence(b_parser *p, b_precedence precedence);
// --> Forward declarations end

static void binary(b_parser *p, b_token previous, bool can_assign) {
  b_tkn_type op = p->previous.type;

  // compile the right operand
  b_parse_rule *rule = get_rule(op);
  parse_precedence(p, (b_precedence) (rule->precedence + 1));

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
      emit_byte(p, OP_F_DIVIDE);
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

      // bitwise
    case AMP_TOKEN:
      emit_byte(p, OP_AND);
      break;

    case BAR_TOKEN:
      emit_byte(p, OP_OR);
      break;

    case XOR_TOKEN:
      emit_byte(p, OP_XOR);
      break;

    case LSHIFT_TOKEN:
      emit_byte(p, OP_LSHIFT);
      break;

    case RSHIFT_TOKEN:
      emit_byte(p, OP_RSHIFT);
      break;

      // range
    case RANGE_TOKEN:
      emit_byte(p, OP_RANGE);
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

static void call(b_parser *p, b_token previous, bool can_assign) {
  uint8_t arg_count = argument_list(p);
  emit_bytes(p, OP_CALL, arg_count);
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

static void parse_assignment(b_parser *p, uint8_t real_op, uint8_t get_op, uint8_t set_op, int arg) {
  p->repl_can_echo = false;
  if (get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY) {
    emit_byte(p, OP_DUP);
  }

  if (arg != -1) {
    emit_byte_and_short(p, get_op, arg);
  } else {
    emit_bytes(p, get_op, 1);
  }

  expression(p);
  emit_byte(p, real_op);
  if (arg != -1) {
    emit_byte_and_short(p, set_op, (uint16_t) arg);
  } else {
    emit_byte(p, set_op);
  }
}

static void assignment(b_parser *p, uint8_t get_op, uint8_t set_op, int arg, bool can_assign) {

  if (can_assign && match(p, EQUAL_TOKEN)) {
    p->repl_can_echo = false;
    expression(p);
    if (arg != -1) {
      emit_byte_and_short(p, set_op, (uint16_t) arg);
    } else {
      emit_byte(p, set_op);
    }
  } else if (can_assign && match(p, PLUS_EQ_TOKEN)) {
    parse_assignment(p, OP_ADD, get_op, set_op, arg);
  } else if (can_assign && match(p, MINUS_EQ_TOKEN)) {
    parse_assignment(p, OP_SUBTRACT, get_op, set_op, arg);
  } else if (can_assign && match(p, MULTIPLY_EQ_TOKEN)) {
    parse_assignment(p, OP_MULTIPLY, get_op, set_op, arg);
  } else if (can_assign && match(p, DIVIDE_EQ_TOKEN)) {
    parse_assignment(p, OP_DIVIDE, get_op, set_op, arg);
  } else if (can_assign && match(p, POW_EQ_TOKEN)) {
    parse_assignment(p, OP_POW, get_op, set_op, arg);
  } else if (can_assign && match(p, PERCENT_EQ_TOKEN)) {
    parse_assignment(p, OP_REMINDER, get_op, set_op, arg);
  } else if (can_assign && match(p, FLOOR_EQ_TOKEN)) {
    parse_assignment(p, OP_F_DIVIDE, get_op, set_op, arg);
  } else if (can_assign && match(p, AMP_EQ_TOKEN)) {
    parse_assignment(p, OP_AND, get_op, set_op, arg);
  } else if (can_assign && match(p, BAR_EQ_TOKEN)) {
    parse_assignment(p, OP_OR, get_op, set_op, arg);
  } else if (can_assign && match(p, TILDE_EQ_TOKEN)) {
    parse_assignment(p, OP_BIT_NOT, get_op, set_op, arg);
  } else if (can_assign && match(p, XOR_EQ_TOKEN)) {
    parse_assignment(p, OP_XOR, get_op, set_op, arg);
  } else if (can_assign && match(p, LSHIFT_EQ_TOKEN)) {
    parse_assignment(p, OP_LSHIFT, get_op, set_op, arg);
  } else if (can_assign && match(p, RSHIFT_EQ_TOKEN)) {
    parse_assignment(p, OP_RSHIFT, get_op, set_op, arg);
  } else if (can_assign && match(p, INCREMENT_TOKEN)) {
    p->repl_can_echo = false;
    if (get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY) {
      emit_byte(p, OP_DUP);
    }

    if (arg != -1) {
      emit_byte_and_short(p, get_op, arg);
    } else {
      emit_bytes(p, get_op, 1);
    }

    emit_bytes(p, OP_ONE, OP_ADD);
    emit_byte_and_short(p, set_op, (uint16_t) arg);
  } else if (can_assign && match(p, DECREMENT_TOKEN)) {
    p->repl_can_echo = false;
    if (get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY) {
      emit_byte(p, OP_DUP);
    }

    if (arg != -1) {
      emit_byte_and_short(p, get_op, arg);
    } else {
      emit_bytes(p, get_op, 1);
    }

    emit_bytes(p, OP_ONE, OP_SUBTRACT);
    emit_byte_and_short(p, set_op, (uint16_t) arg);
  } else {
    if (arg != -1) {
      if (get_op == OP_GET_INDEX || get_op == OP_GET_RANGED_INDEX) {
        emit_bytes(p, get_op, (uint8_t) 0);
      } else {
        emit_byte_and_short(p, get_op, (uint16_t) arg);
      }
    } else {
      emit_bytes(p, get_op, (uint8_t) 0);
    }
  }
}

static void dot(b_parser *p, b_token previous, bool can_assign) {
  ignore_whitespace(p);
  consume(p, IDENTIFIER_TOKEN, "expected property name after '.'");
  int name = identifier_constant(p, &p->previous);

  if (match(p, LPAREN_TOKEN)) {
    uint8_t arg_count = argument_list(p);
    if (p->current_class != NULL && (previous.type == SELF_TOKEN
                                     || identifiers_equal(&p->previous, &p->current_class->name))) {
      emit_byte_and_short(p, OP_INVOKE_SELF, name);
    } else {
      emit_byte_and_short(p, OP_INVOKE, name);
    }
    emit_byte(p, arg_count);
  } else {
    b_code get_op = OP_GET_PROPERTY, set_op = OP_SET_PROPERTY;

    if (p->current_class != NULL && (previous.type == SELF_TOKEN
                                     || identifiers_equal(&p->previous, &p->current_class->name))) {
      get_op = OP_GET_SELF_PROPERTY;
    }

    assignment(p, get_op, set_op, name, can_assign);
  }
}

static void named_variable(b_parser *p, b_token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(p, p->vm->compiler, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if ((arg = resolve_up_value(p, p->vm->compiler, &name)) != -1) {
    get_op = OP_GET_UP_VALUE;
    set_op = OP_SET_UP_VALUE;
  } else {
    arg = identifier_constant(p, &name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  assignment(p, get_op, set_op, arg, can_assign);
}

static void created_variable(b_parser *p, b_token name) {
  uint8_t get_op, set_op;
  int arg;
  if (p->vm->compiler->function->name != NULL) {
    int local = add_local(p, name) - 1;
    mark_initialized(p);
    emit_byte_and_short(p, OP_SET_LOCAL, (uint16_t)local);
  } else {
    emit_byte_and_short(p, OP_DEFINE_GLOBAL, (uint16_t) identifier_constant(p, &name));
  }
}

static void list(b_parser *p, bool can_assign) {
  emit_byte(p, OP_NIL); // placeholder for the list

  int count = 0;
  ignore_whitespace(p);
  if (!check(p, RBRACKET_TOKEN)) {
    do {
      ignore_whitespace(p);

      if(!check(p, RBRACKET_TOKEN)) { // allow comma to end lists
        expression(p);
        ignore_whitespace(p);
        count++;
      }
    } while (match(p, COMMA_TOKEN));
  }
  ignore_whitespace(p);
  consume(p, RBRACKET_TOKEN, "expected ']' at end of list");

  emit_byte_and_short(p, OP_LIST, count);
}

static void dictionary(b_parser *p, bool can_assign) {
  emit_byte(p, OP_NIL); // placeholder for the dictionary

  int item_count = 0;
  ignore_whitespace(p);
  if (!check(p, RBRACE_TOKEN)) {
    do {
      ignore_whitespace(p);

      if (!check(p, RBRACE_TOKEN)) { // allow last pair to end with a comma
        bool used_expression = false;
        if (check(p, IDENTIFIER_TOKEN)) {
          consume(p, IDENTIFIER_TOKEN, "");
          emit_constant(p, OBJ_VAL(copy_string(p->vm, p->previous.start, p->previous.length)));
        } else {
          expression(p);
          used_expression = true;
        }
        ignore_whitespace(p);
        if(!check(p, COMMA_TOKEN) && !check(p, RBRACE_TOKEN)) {
          consume(p, COLON_TOKEN, "expected ':' after dictionary key");
          ignore_whitespace(p);

          expression(p);
        } else {
          if(used_expression) {
            error(p, "cannot infer dictionary values from expressions");
          } else {
            named_variable(p, p->previous, false);
          }
        }
        item_count++;
      }
    } while (match(p, COMMA_TOKEN));
  }
  ignore_whitespace(p);
  consume(p, RBRACE_TOKEN, "expected '}' after dictionary");

  emit_byte_and_short(p, OP_DICT, item_count);
}

static void indexing(b_parser *p, b_token previous, bool can_assign) {
  bool assignable = true, comma_match = false;
  uint8_t get_op = OP_GET_INDEX;
  if (match(p, COMMA_TOKEN)) {
    emit_byte(p, OP_NIL);
    comma_match = true;
    get_op = OP_GET_RANGED_INDEX;
  } else {
    expression(p);
  }

  if (!match(p, RBRACKET_TOKEN)) {
    get_op = OP_GET_RANGED_INDEX;
    if (!comma_match) {
      consume(p, COMMA_TOKEN, "expecting ',' or ']'");
    }
    if (match(p, RBRACKET_TOKEN)) {
      emit_byte(p, OP_NIL);
    } else {
      expression(p);
      consume(p, RBRACKET_TOKEN, "expected ']' after indexing");
    }
    assignable = false;
  } else {
    if(comma_match) {
      emit_byte(p, OP_NIL);
    }
  }

  assignment(p, get_op, OP_SET_INDEX, -1, assignable);
}

static void variable(b_parser *p, bool can_assign) {
  named_variable(p, p->previous, can_assign);
}

static void self(b_parser *p, bool can_assign) {
  if (p->current_class == NULL) {
    error(p, "cannot use keyword 'self' outside of a class");
    return;
  }
  variable(p, false);
}

static void parent(b_parser *p, bool can_assign) {
  if (p->current_class == NULL) {
    error(p, "cannot use keyword 'parent' outside of a class");
  } else if (!p->current_class->has_superclass) {
    error(p, "cannot use keyword 'parent' in a class without a parent");
  }

  int name = -1;
  bool invoke_self = false;

  if (!check(p, LPAREN_TOKEN)) {
    consume(p, DOT_TOKEN, "expected '.' or '(' after parent");
    consume(p, IDENTIFIER_TOKEN, "expected parent class method name after .");
    name = identifier_constant(p, &p->previous);
  } else {
    invoke_self = true;
  }

  named_variable(p, synthetic_token("self"), false);

  if (match(p, LPAREN_TOKEN)) {
    uint8_t arg_count = argument_list(p);
    named_variable(p, synthetic_token("parent"), false);
    if (!invoke_self) {
      emit_byte_and_short(p, OP_SUPER_INVOKE, name);
      emit_byte(p, arg_count);
    } else {
      emit_bytes(p, OP_SUPER_INVOKE_SELF, arg_count);
    }
  } else {
    named_variable(p, synthetic_token("parent"), false);
    emit_byte_and_short(p, OP_GET_SUPER, name);
  }
}

static void grouping(b_parser *p, bool can_assign) {
  ignore_whitespace(p);
  expression(p);
  ignore_whitespace(p);
  consume(p, RPAREN_TOKEN, "expected ')' after grouped expression");
}

static b_value compile_number(b_parser *p) {
  if (p->previous.type == BIN_NUMBER_TOKEN) {
    long long value = strtoll(p->previous.start + 2, NULL, 2);
    return NUMBER_VAL(value);
  } else if (p->previous.type == OCT_NUMBER_TOKEN) {
    long value = strtol(p->previous.start + 2, NULL, 8);
    return NUMBER_VAL(value);
  } else if (p->previous.type == HEX_NUMBER_TOKEN) {
    long value = strtol(p->previous.start, NULL, 16);
    return NUMBER_VAL(value);
  } else {
    double value = strtod(p->previous.start, NULL);
    return NUMBER_VAL(value);
  }
}

static void number(b_parser *p, bool can_assign) {
  emit_constant(p, compile_number(p));
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
    digit = read_hex_digit(str[index + i + 2]);
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
    char *chr = utf8_encode(value);
    if(chr) {
      memcpy(string + index, chr, (size_t) count + 1);
      free(chr);
    } else {
      error(p, "cannot decode unicode escape at index %d", real_index);
    }
  }
  /* if (value > 65535) // but greater than \uffff doesn't occupy any extra byte
    count--; */
  return count;
}

static char *compile_string(b_parser *p, int *length) {
  char *str = (char *) malloc((((size_t) p->previous.length - 2) + 1) * sizeof(char));
  char quote = p->previous.start[0];
  char *real = (char *) p->previous.start + 1;

  int real_length = p->previous.length - 2, k = 0;

  for (int i = 0; i < real_length; i++, k++) {
    char c = real[i];
    if (c == '\\' && i < real_length - 1) {
      switch (real[i + 1]) {
        case '0':
          c = '\0';
          break;
        case '$':
          c = '$';
          break;
        case '\'':
          if(quote == '\'' || quote == '}') c = '\''; // } handle closing of interpolation.
          else i--;
          break;
        case '"':
          if(quote == '"' || quote == '}') c = '"';
          else i--;
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
          int count = read_unicode_escape(p, str, real, 8, i, k);
          k += count > 4 ? count - 2 : count - 1;
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

  *length = k;
  str[k] = '\0';
  return str;
}

static void string(b_parser *p, bool can_assign) {
  int length;
  char *str = compile_string(p, &length);
  emit_constant(p, OBJ_VAL(take_string(p->vm, str, length)));
}

static void string_interpolation(b_parser *p, bool can_assign) {
  int count = 0;
  do {
    bool do_add = false;
    bool string_matched = false;

    if (p->previous.length - 2 > 0) {
      string(p, can_assign);
      do_add = true;
      string_matched = true;

      if (count > 0) {
        emit_byte(p, OP_ADD);
      }
    }

    expression(p);
    emit_byte(p, OP_STRINGIFY);

    if (do_add || (count >= 1 && string_matched == false)) {
      emit_byte(p, OP_ADD);
    }
    count++;
  } while (match(p, INTERPOLATION_TOKEN));

  consume(p, LITERAL_TOKEN, "unterminated string interpolation");

  if (p->previous.length - 2 > 0) {
    string(p, can_assign);
    emit_byte(p, OP_ADD);
  }
}

static void unary(b_parser *p, bool can_assign) {
  b_tkn_type op = p->previous.type;

  // compile the expression
  parse_precedence(p, PREC_UNARY);

  // emit instruction
  switch (op) {
    case MINUS_TOKEN:
      emit_byte(p, OP_NEGATE);
      break;
    case BANG_TOKEN:
      emit_byte(p, OP_NOT);
      break;
    case TILDE_TOKEN:
      emit_byte(p, OP_BIT_NOT);
      break;

    default:
      break;
  }
}

static void and_(b_parser *p, b_token previous, bool can_assign) {
  int end_jump = emit_jump(p, OP_JUMP_IF_FALSE);

  emit_byte(p, OP_POP);
  parse_precedence(p, PREC_AND);

  patch_jump(p, end_jump);
}

static void or_(b_parser *p, b_token previous, bool can_assign) {
  int else_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(p, OP_JUMP);

  patch_jump(p, else_jump);
  emit_byte(p, OP_POP);

  parse_precedence(p, PREC_OR);
  patch_jump(p, end_jump);
}

static void conditional(b_parser *p, b_token previous, bool can_assign) {
  int then_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);

  ignore_whitespace(p);
  // compile the then expression
  parse_precedence(p, PREC_CONDITIONAL);
  ignore_whitespace(p);

  int else_jump = emit_jump(p, OP_JUMP);

  patch_jump(p, then_jump);
  emit_byte(p, OP_POP);


  consume(p, COLON_TOKEN, "expected matching ':' after '?' conditional");
  ignore_whitespace(p);
  // compile the else expression
  // here we parse at PREC_ASSIGNMENT precedence as
  // linear conditionals can be nested.
  parse_precedence(p, PREC_ASSIGNMENT);

  patch_jump(p, else_jump);
}

b_parse_rule parse_rules[] = {
    // symbols
    [NEWLINE_TOKEN] = {NULL, NULL, PREC_NONE},                // (
    [LPAREN_TOKEN] = {grouping, call, PREC_CALL},             // (
    [RPAREN_TOKEN] = {NULL, NULL, PREC_NONE},                 // )
    [LBRACKET_TOKEN] = {list, indexing, PREC_CALL},           // [
    [RBRACKET_TOKEN] = {NULL, NULL, PREC_NONE},               // ]
    [LBRACE_TOKEN] = {dictionary, NULL, PREC_NONE},           // {
    [RBRACE_TOKEN] = {NULL, NULL, PREC_NONE},                 // }
    [SEMICOLON_TOKEN] = {NULL, NULL, PREC_NONE},              // ;
    [COMMA_TOKEN] = {NULL, NULL, PREC_NONE},                  // ,
    [BACKSLASH_TOKEN] = {NULL, NULL, PREC_NONE},              // '\'
    [BANG_TOKEN] = {unary, NULL, PREC_NONE},                  // !
    [BANG_EQ_TOKEN] = {NULL, binary, PREC_EQUALITY},          // !=
    [COLON_TOKEN] = {NULL, NULL, PREC_NONE},                  // :
    [AT_TOKEN] = {anonymous, NULL, PREC_NONE},                     // @
    [DOT_TOKEN] = {NULL, dot, PREC_CALL},                     // .
    [RANGE_TOKEN] = {NULL, binary, PREC_RANGE},               // ..
    [TRI_DOT_TOKEN] = {NULL, NULL, PREC_NONE},                // ...
    [PLUS_TOKEN] = {unary, binary, PREC_TERM},                 // +
    [PLUS_EQ_TOKEN] = {NULL, NULL, PREC_NONE},                // +=
    [INCREMENT_TOKEN] = {NULL, NULL, PREC_NONE},              // ++
    [MINUS_TOKEN] = {unary, binary, PREC_TERM},               // -
    [MINUS_EQ_TOKEN] = {NULL, NULL, PREC_NONE},               // -=
    [DECREMENT_TOKEN] = {NULL, NULL, PREC_NONE},              // --
    [MULTIPLY_TOKEN] = {NULL, binary, PREC_FACTOR},           // *
    [MULTIPLY_EQ_TOKEN] = {NULL, NULL, PREC_NONE},            // *=
    [POW_TOKEN] = {NULL, binary, PREC_FACTOR},                // **
    [POW_EQ_TOKEN] = {NULL, NULL, PREC_NONE},                 // **=
    [DIVIDE_TOKEN] = {NULL, binary, PREC_FACTOR},             // '/'
    [DIVIDE_EQ_TOKEN] = {NULL, NULL, PREC_NONE},              // '/='
    [FLOOR_TOKEN] = {NULL, binary, PREC_FACTOR},              // '//'
    [FLOOR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},               // '//='
    [EQUAL_TOKEN] = {NULL, NULL, PREC_NONE},                  // =
    [EQUAL_EQ_TOKEN] = {NULL, binary, PREC_EQUALITY},         // ==
    [LESS_TOKEN] = {NULL, binary, PREC_COMPARISON},           // <
    [LESS_EQ_TOKEN] = {NULL, binary, PREC_COMPARISON},        // <=
    [LSHIFT_TOKEN] = {NULL, binary, PREC_SHIFT},              // <<
    [LSHIFT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},              // <<=
    [GREATER_TOKEN] = {NULL, binary, PREC_COMPARISON},        // >
    [GREATER_EQ_TOKEN] = {NULL, binary, PREC_COMPARISON},     // >=
    [RSHIFT_TOKEN] = {NULL, binary, PREC_SHIFT},              // >>
    [RSHIFT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},              // >>=
    [PERCENT_TOKEN] = {NULL, binary, PREC_FACTOR},            // %
    [PERCENT_EQ_TOKEN] = {NULL, NULL, PREC_NONE},             // %=
    [AMP_TOKEN] = {NULL, binary, PREC_BIT_AND},               // &
    [AMP_EQ_TOKEN] = {NULL, NULL, PREC_NONE},                 // &=
    [BAR_TOKEN] = {NULL, binary, PREC_BIT_OR},           // |
    [BAR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},                 // |=
    [TILDE_TOKEN] = {unary, NULL, PREC_UNARY},                // ~
    [TILDE_EQ_TOKEN] = {NULL, NULL, PREC_NONE},               // ~=
    [XOR_TOKEN] = {NULL, binary, PREC_BIT_XOR},               // ^
    [XOR_EQ_TOKEN] = {NULL, NULL, PREC_NONE},                 // ^=
    [QUESTION_TOKEN] = {NULL, conditional, PREC_CONDITIONAL}, // ??

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
    [DO_TOKEN] = {NULL, NULL, PREC_NONE},
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
    [PARENT_TOKEN] = {parent, NULL, PREC_NONE},
    [RETURN_TOKEN] = {NULL, NULL, PREC_NONE},
    [SELF_TOKEN] = {self, NULL, PREC_NONE},
    [STATIC_TOKEN] = {NULL, NULL, PREC_NONE},
    [TRUE_TOKEN] = {literal, NULL, PREC_NONE},
    [USING_TOKEN] = {NULL, NULL, PREC_NONE},
    [WHEN_TOKEN] = {NULL, NULL, PREC_NONE},
    [WHILE_TOKEN] = {NULL, NULL, PREC_NONE},
    [TRY_TOKEN] = {NULL, NULL, PREC_NONE},
    [CATCH_TOKEN] = {NULL, NULL, PREC_NONE},
    [FINALLY_TOKEN] = {NULL, NULL, PREC_NONE},

    // types token
    [LITERAL_TOKEN] = {string, NULL, PREC_NONE},
    [REG_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // regular numbers
    [BIN_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // binary numbers
    [OCT_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // octal numbers
    [HEX_NUMBER_TOKEN] = {number, NULL, PREC_NONE}, // hexadecimal numbers
    [IDENTIFIER_TOKEN] = {variable, NULL, PREC_NONE},
    [INTERPOLATION_TOKEN] = {string_interpolation, NULL, PREC_NONE},
    [EOF_TOKEN] = {NULL, NULL, PREC_NONE},

    // error
    [ERROR_TOKEN] = {NULL, NULL, PREC_NONE},
    [EMPTY_TOKEN] = {literal, NULL, PREC_NONE},
    [UNDEFINED_TOKEN] = {NULL, NULL, PREC_NONE},
};

static void do_parse_precedence(b_parser *p, b_precedence precedence) {
  b_parse_prefix_fn prefix_rule = get_rule(p->previous.type)->prefix;

  if (prefix_rule == NULL) {
    error(p, "expected expression");
    return;
  }

  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(p, can_assign);

  while (precedence <= get_rule(p->current.type)->precedence) {
    b_token previous = p->previous;
    ignore_whitespace(p);
    advance(p);
    b_parse_infix_fn infix_rule = get_rule(p->previous.type)->infix;
    infix_rule(p, previous, can_assign);
  }

  if (can_assign && match(p, EQUAL_TOKEN)) {
    error(p, "invalid assignment target");
  }
}

static void parse_precedence(b_parser *p, b_precedence precedence) {
  if (is_at_end(p->scanner) && p->vm->is_repl)
    return;

  ignore_whitespace(p);

  if (is_at_end(p->scanner) && p->vm->is_repl)
    return;

  advance(p);

  do_parse_precedence(p, precedence);
}

static void parse_precedence_no_advance(b_parser *p, b_precedence precedence) {
  if (is_at_end(p->scanner) && p->vm->is_repl)
    return;

  ignore_whitespace(p);

  if (is_at_end(p->scanner) && p->vm->is_repl)
    return;

  do_parse_precedence(p, precedence);
}

static b_parse_rule *get_rule(b_tkn_type type) { return &parse_rules[type]; }

static void expression(b_parser *p) { parse_precedence(p, PREC_ASSIGNMENT); }

static void block(b_parser *p) {
  p->block_count++;
  ignore_whitespace(p);
  while (!check(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    declaration(p);
  }
  p->block_count--;
  consume(p, RBRACE_TOKEN, "expected '}' after block");
}

static void function_args(b_parser *p) {
  // compile argument list...
  do {
    ignore_whitespace(p);
    p->vm->compiler->function->arity++;
    if (p->vm->compiler->function->arity > MAX_FUNCTION_PARAMETERS) {
      error_at_current(p, "cannot have more than %d function parameters",
                       MAX_FUNCTION_PARAMETERS);
    }

    if (match(p, TRI_DOT_TOKEN)) {
      p->vm->compiler->function->is_variadic = true;
      add_local(p, synthetic_token("__args__"));
      define_variable(p, 0);
      break;
    }

    int param_constant = parse_variable(p, "expected parameter name");
    define_variable(p, param_constant);
    ignore_whitespace(p);
  } while (match(p, COMMA_TOKEN));
}

static void function_body(b_parser *p, b_compiler *compiler, bool close_scope) {
  // compile the body
  ignore_whitespace(p);
  consume(p, LBRACE_TOKEN, "expected '{' before function body");
  block(p);

  // create the function object
  if(close_scope) {
      end_scope(p);
  }
  b_obj_func *function = end_compiler(p);

  push(p->vm, OBJ_VAL(function));
  emit_byte_and_short(p, OP_CLOSURE, make_constant(p, OBJ_VAL(function)));

  for (int i = 0; i < function->up_value_count; i++) {
    emit_byte(p, compiler->up_values[i].is_local ? 1 : 0);
    emit_short(p, compiler->up_values[i].index);
  }

  pop(p->vm);
}

static void function(b_parser *p, b_func_type type) {
  b_compiler compiler;
  init_compiler(p, &compiler, type);
  begin_scope(p);

  // compile parameter list
  consume(p, LPAREN_TOKEN, "expected '(' after function name");
  if (!check(p, RPAREN_TOKEN)) {
    function_args(p);
  }
  consume(p, RPAREN_TOKEN, "expected ')' after function parameters");

  function_body(p, &compiler, false);
}

static void method(b_parser *p, b_token class_name, bool is_static) {
  b_tkn_type tkns[] = {IDENTIFIER_TOKEN, DECORATOR_TOKEN};

  consume_or(p, "method name expected", tkns, 2);
  int constant = identifier_constant(p, &p->previous);

  b_func_type type = is_static ? TYPE_STATIC : TYPE_METHOD;
  if (p->previous.length == class_name.length &&
      memcmp(p->previous.start, class_name.start, class_name.length) == 0) {
    type = TYPE_INITIALIZER;
  } else if (p->previous.length > 0 && p->previous.start[0] == '_') {
    type = TYPE_PRIVATE;
  }

  function(p, type);
  emit_byte_and_short(p, OP_METHOD, constant);
}

static void anonymous(b_parser *p, bool can_assign) {
  b_compiler compiler;
  init_compiler(p, &compiler, TYPE_FUNCTION);
  begin_scope(p);

  // compile parameter list
  if(check(p, LPAREN_TOKEN)) {
    consume(p, LPAREN_TOKEN, "expected '(' at start of anonymous function");
    if (!check(p, RPAREN_TOKEN)) {
      function_args(p);
    }
    consume(p, RPAREN_TOKEN, "expected ')' after anonymous function parameters");
  }

  function_body(p, &compiler, true);
}

static void field(b_parser *p, bool is_static) {
  consume(p, IDENTIFIER_TOKEN, "class property name expected");
  int field_constant = identifier_constant(p, &p->previous);

  if (match(p, EQUAL_TOKEN)) {
    expression(p);
  } else {
    emit_byte(p, OP_NIL);
  }

  emit_byte_and_short(p, OP_CLASS_PROPERTY, field_constant);
  emit_byte(p, is_static ? 1 : 0);

  consume_statement_end(p);
  ignore_whitespace(p);
}

static void function_declaration(b_parser *p) {
  int global = parse_variable(p, "function name expected");
  mark_initialized(p);
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

  b_class_compiler class_compiler;
  class_compiler.name = p->previous;
  class_compiler.has_superclass = false;
  class_compiler.enclosing = p->current_class;
  p->current_class = &class_compiler;

  if (match(p, LESS_TOKEN)) {
    consume(p, IDENTIFIER_TOKEN, "name of superclass expected");
    variable(p, false);

    if (identifiers_equal(&class_name, &p->previous)) {
      error(p, "class %.*s cannot inherit from itself", class_name.length, class_name.start);
    }

    begin_scope(p);
    add_local(p, synthetic_token("parent"));
    define_variable(p, 0);

    named_variable(p, class_name, false);
    emit_byte(p, OP_INHERIT);
    class_compiler.has_superclass = true;
  }

  named_variable(p, class_name, false);

  ignore_whitespace(p);
  consume(p, LBRACE_TOKEN, "expected '{' before class body");
  ignore_whitespace(p);
  while (!check(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    bool is_static = false;
    if (match(p, STATIC_TOKEN))
      is_static = true;

    if (match(p, VAR_TOKEN)) {
      field(p, is_static);
    } else {
      method(p, class_name, is_static);
      ignore_whitespace(p);
    }
  }
  consume(p, RBRACE_TOKEN, "expected '}' after class body");
  emit_byte(p, OP_POP);

  if (class_compiler.has_superclass) {
    end_scope(p);
  }

  p->current_class = p->current_class->enclosing;
}

static void compile_var_declaration(b_parser *p, bool is_initializer) {
  int total_parsed = 0;

  do {
    if(total_parsed > 0) {
      ignore_whitespace(p);
    }
    int global = parse_variable(p, "variable name expected");

    if (match(p, EQUAL_TOKEN)) {
      expression(p);
    } else {
      emit_byte(p, OP_NIL);
    }

    define_variable(p, global);
    total_parsed++;
  } while (match(p, COMMA_TOKEN));

  if (!is_initializer) {
    consume_statement_end(p);
  } else {
    consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
    ignore_whitespace(p);
  }
}

static void var_declaration(b_parser *p) { compile_var_declaration(p, false); }

static void expression_statement(b_parser *p, bool is_initializer, bool semi) {
  if (p->vm->is_repl && p->vm->compiler->scope_depth == 0) {
    p->repl_can_echo = true;
  }
  if(!semi) {
    expression(p);
  } else {
    parse_precedence_no_advance(p, PREC_ASSIGNMENT);
  }
  if (!is_initializer) {
    if (p->repl_can_echo && p->vm->is_repl) {
      emit_byte(p, OP_ECHO);
      p->repl_can_echo = false;
    } else {
      emit_byte(p, OP_POP);
    }
    consume_statement_end(p);
  } else {
    consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
    ignore_whitespace(p);
    emit_byte(p, OP_POP);
  }
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

  // parse initializer...
  if (match(p, SEMICOLON_TOKEN)) {
    // no initializer
  } else if (match(p, VAR_TOKEN)) {
    compile_var_declaration(p, true);
  } else {
    expression_statement(p, true, false);
  }

  // keep a copy of the surrounding loop's start and depth
  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // update the parser's loop start and depth to the current
  p->innermost_loop_start = current_blob(p)->count;
  p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

  int exit_jump = -1;
  if (!match(p, SEMICOLON_TOKEN)) { // the condition is optional
    expression(p);
    consume(p, SEMICOLON_TOKEN, "expected ';' after condition");
    ignore_whitespace(p);

    // jump out of the loop if the condition is false...
    exit_jump = emit_jump(p, OP_JUMP_IF_FALSE);
    emit_byte(p, OP_POP); // pop the condition
  }

  // the iterator...
  if (!check(p, LBRACE_TOKEN)) {
    int body_jump = emit_jump(p, OP_JUMP);

    int increment_start = current_blob(p)->count;
    expression(p);
    ignore_whitespace(p);
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
 * for x in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var _
 *
 *    while _ = iterable.@itern() {
 *      var x = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * ---------------------------------
 *
 * for x, y in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var x
 *
 *    while x = iterable.@itern() {
 *      var y = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * Every blade iterable must implement the @iter(x) and the @itern(x)
 * function.
 *
 * to make instances of a user created class iterable,
 * the class must implement the @iter(x) and the @itern(x) function.
 * the @itern(x) must return the current iterating index of the object and
 * the
 * @iter(x) function must return the value at that index.
 * _NOTE_: the @iter(x) function will no longer be called after the
 * @itern(x) function returns a false value. so the @iter(x) never needs
 * to return a false value
 */
static void for_statement(b_parser *p) {
  begin_scope(p);

  // define @iter and @itern constant
  int iter__ = make_constant(p, OBJ_VAL(copy_string(p->vm, "@iter", 5)));
  int iter_n__ = make_constant(p, OBJ_VAL(copy_string(p->vm, "@itern", 6)));

  consume(p, IDENTIFIER_TOKEN, "expected variable name after 'for'");
  b_token key_token, value_token;

  if (!check(p, COMMA_TOKEN)) {
    key_token = synthetic_token(" _ ");
    value_token = p->previous;
  } else {
    key_token = p->previous;

    consume(p, COMMA_TOKEN, "");
    consume(p, IDENTIFIER_TOKEN, "expected variable name after ','");
    value_token = p->previous;
  }

  consume(p, IN_TOKEN, "expected 'in' after for loop variable(s)");
  ignore_whitespace(p);

  // The space in the variable name ensures it won't collide with a user-defined
  // variable.
  b_token iterator_token = synthetic_token(" iterator ");

  // Evaluate the sequence expression and store it in a hidden local variable.
  expression(p);

  if (p->vm->compiler->local_count + 3 > UINT8_COUNT) {
    error(p, "cannot declare more than %d variables in one scope", UINT8_COUNT);
    return;
  }

  // add the iterator to the local scope
  int iterator_slot = add_local(p, iterator_token) - 1;
  define_variable(p, 0);

  // Create the key local variable.
  emit_byte(p, OP_NIL);
  int key_slot = add_local(p, key_token) - 1;
  define_variable(p, key_slot);

  // create the local value slot
  emit_byte(p, OP_NIL);
  int value_slot = add_local(p, value_token) - 1;
  define_variable(p, 0);

  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // we'll be jumping back to right before the
  // expression after the loop body
  p->innermost_loop_start = current_blob(p)->count;
  p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

  // key = iterable.iter_n__(key)
  emit_byte_and_short(p, OP_GET_LOCAL, iterator_slot);
  emit_byte_and_short(p, OP_GET_LOCAL, key_slot);
  emit_byte_and_short(p, OP_INVOKE, iter_n__);
  emit_byte(p, 1);
  emit_byte_and_short(p, OP_SET_LOCAL, key_slot);

  int false_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);

  // value = iterable.iter__(key)
  emit_byte_and_short(p, OP_GET_LOCAL, iterator_slot);
  emit_byte_and_short(p, OP_GET_LOCAL, key_slot);
  emit_byte_and_short(p, OP_INVOKE, iter__);
  emit_byte(p, 1);

  // Bind the loop value in its own scope. This ensures we get a fresh
  // variable each iteration so that closures for it don't all see the same one.
  begin_scope(p);

  // update the value
  emit_byte_and_short(p, OP_SET_LOCAL, value_slot);
  emit_byte(p, OP_POP);

  statement(p);

  end_scope(p);

  emit_loop(p, p->innermost_loop_start);

  patch_jump(p, false_jump);
  emit_byte(p, OP_POP);

  end_loop(p);

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

  b_obj_switch *sw = new_switch(p->vm);
  push(p->vm, OBJ_VAL(sw));

  int switch_code = emit_switch(p);
  // emit_byte_and_short(p, OP_SWITCH, make_constant(p, OBJ_VAL(sw)));
  int start_offset = current_blob(p)->count;

  while (!match(p, RBRACE_TOKEN) && !check(p, EOF_TOKEN)) {
    if (match(p, WHEN_TOKEN) || match(p, DEFAULT_TOKEN)) {
      b_tkn_type case_type = p->previous.type;

      if (state == 2) {
        error(p, "cannot have another case after a default case");
      }

      if (state == 1) {
        // at the end of the previous case, jump over the others...
        case_ends[case_count++] = emit_jump(p, OP_JUMP);
      }

      if (case_type == WHEN_TOKEN) {
        state = 1;
        do {
          ignore_whitespace(p);

          advance(p);

          b_value jump = NUMBER_VAL((double) current_blob(p)->count - (double) start_offset);

          if (p->previous.type == TRUE_TOKEN) {
            table_set(p->vm, &sw->table, TRUE_VAL, jump);
          } else if (p->previous.type == FALSE_TOKEN) {
            table_set(p->vm, &sw->table, FALSE_VAL, jump);
          } else if (p->previous.type == LITERAL_TOKEN) {
            int length;
            char *str = compile_string(p, &length);
            b_obj_string *string = take_string(p->vm, str, length);
            push(p->vm, OBJ_VAL(string)); // gc fix
            table_set(p->vm, &sw->table, OBJ_VAL(string), jump);
            pop(p->vm); // gc fix
          } else if (check_number(p)) {
            table_set(p->vm, &sw->table, compile_number(p), jump);
          } else {
            pop(p->vm); // pop the switch
            error(p, "only constants can be used in when expressions");
            return;
          }
        } while(match(p, COMMA_TOKEN));
      } else {
        state = 2;
        sw->default_jump = current_blob(p)->count - start_offset;
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
    case_ends[case_count++] = emit_jump(p, OP_JUMP);
  }

  // patch all the case jumps to the end
  for (int i = 0; i < case_count; i++) {
    patch_jump(p, case_ends[i]);
  }

  sw->exit_jump = current_blob(p)->count - start_offset;

  patch_with_value(p, switch_code, make_constant(p, OBJ_VAL(sw)));
  pop(p->vm); // pop the switch
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
  emit_byte(p, OP_ECHO);
  consume_statement_end(p);
}

static void die_statement(b_parser *p) {
  discard_locals(p, p->vm->compiler->scope_depth);
  expression(p);
  emit_byte(p, OP_DIE);
  consume_statement_end(p);
}

static void parse_specific_import(b_parser *p, char *module_name, int import_constant, bool was_renamed, bool is_native) {
  if (match(p, LBRACE_TOKEN)) {
    if (was_renamed) {
      error(p, "selective import on renamed module");
      return;
    }

    emit_byte_and_short(p, OP_CONSTANT, import_constant);
    bool same_name_selective_exist = false;

    do {
      ignore_whitespace(p);

      // terminate on all (*)
      if (match(p, MULTIPLY_TOKEN)) {
        emit_byte(p, is_native ? OP_IMPORT_ALL_NATIVE : OP_IMPORT_ALL);
        break;
      }

      int name = parse_variable(p, "module object name expected");

      if(module_name != NULL && p->previous.length == (int)strlen(module_name) &&
          memcmp(module_name, p->previous.start, p->previous.length) == 0) {
        same_name_selective_exist = true;
      }

      emit_byte_and_short(p, is_native ? OP_SELECT_NATIVE_IMPORT : OP_SELECT_IMPORT, name);
    } while (match(p, COMMA_TOKEN));
    ignore_whitespace(p);

    consume(p, RBRACE_TOKEN, "expected '}' at end of selective import");

    if(!same_name_selective_exist) {
      emit_byte_and_short(p, is_native ? OP_EJECT_NATIVE_IMPORT : OP_EJECT_IMPORT, import_constant);
    }
    emit_byte(p, OP_POP); // pop the module constant from stack
    consume_statement_end(p);
  }
}

static void import_statement(b_parser *p) {
//  consume(p, LITERAL_TOKEN, "expected module name");
//  int module_name_length;
//  char *module_name = compile_string(p, &module_name_length);

  char *module_name = NULL;
  char *module_file = NULL;

  int part_count = 0;

  bool is_relative = match(p, DOT_TOKEN);

  // allow for import starting with ..
  if (!is_relative) {
    if (match(p, RANGE_TOKEN)) {}
  } else {
    if (match(p, RANGE_TOKEN)) {
      error(p, "conflicting module path. Parent or current directory?");
      return;
    }
  }

  do {
    if (p->previous.type == RANGE_TOKEN) {
      is_relative = true;
      if (module_file == NULL) {
        module_file = strdup("/../");
      } else {
        module_file = append_strings(module_file, "/../");
      }
    }

    if(module_name != NULL) {
      free(module_name);
    }

    consume(p, IDENTIFIER_TOKEN, "module name expected");

    module_name = (char *) calloc(p->previous.length + 1, sizeof(char));
    memcpy(module_name, p->previous.start, p->previous.length);
    module_name[p->previous.length] = '\0';

    // handle native modules
    if (part_count == 0 && module_name[0] == '_' && !is_relative) {
      int module = make_constant(p, OBJ_VAL(copy_string(p->vm, module_name, (int) strlen(module_name))));
      emit_byte_and_short(p, OP_NATIVE_MODULE, module);

      parse_specific_import(p, module_name, module, false, true);
      free(module_name);
      return;
    }

    if (module_file == NULL) {
      module_file = strdup(module_name);
    } else {
      if (module_file[strlen(module_file) - 1] != BLADE_PATH_SEPARATOR[0]) {
        module_file = append_strings(module_file, BLADE_PATH_SEPARATOR);
      }
      module_file = append_strings(module_file, module_name);
    }

    part_count++;
  } while (match(p, DOT_TOKEN) || match(p, RANGE_TOKEN));

  bool was_renamed = false;

  if (match(p, AS_TOKEN)) {
    consume(p, IDENTIFIER_TOKEN, "module name expected");
    free(module_name);
    module_name = (char *) calloc(p->previous.length + 1, sizeof(char));
    if (module_name == NULL) {
      error(p, "could not allocate memory for module name");
      return;
    }
    memcpy(module_name, p->previous.start, p->previous.length);
    module_name[p->previous.length] = '\0';
    was_renamed = true;
  }

  char *module_path = resolve_import_path(module_file, p->module->file, p->vm->root_file, is_relative);

  if (module_path == NULL) {
    // check if there is one in the vm's registry
    // handle native modules
    b_value md;
    b_obj_string *final_module_name = copy_string(p->vm, module_name, (int) strlen(module_name));
    if (table_get(&p->vm->modules, OBJ_VAL(final_module_name), &md)) {
      int module = make_constant(p, OBJ_VAL(final_module_name));
      push(p->vm, OBJ_VAL(final_module_name));
      emit_byte_and_short(p, OP_NATIVE_MODULE, module);
      pop(p->vm);

      parse_specific_import(p, module_name, module, false, true);
      free(module_name);
      return;
    }

    free(module_path);
    free(module_file);
    free(module_name);
    error(p, "module not found");
    return;
  }

  if (!check(p, LBRACE_TOKEN)) {
    consume_statement_end(p);
  }

  // do the import here...
  char *source = read_file(module_path);
  if (source == NULL) {
    error(p, "could not read import file %s", module_path);
    free(module_file);
    return;
  }

  // prevent cyclic imports
  b_obj_module *check_module = p->module;
  while (check_module->parent != NULL) {
    size_t path_length = strlen(check_module->parent->file);
    if(strlen(module_path) == path_length) {
      if(memcmp(module_path, check_module->parent->file, path_length) == 0) {
        error(p, "cyclic import detected at %s", module_name);
        free(module_path);
        return;
      }
    }
    check_module = check_module->parent;
  }

  b_obj_module *module = new_module(p->vm, module_name, module_path, p->module);

  push(p->vm, OBJ_VAL(module));
  b_obj_func *function = compile(p->vm, module, source);
  pop(p->vm);

  free(source);

  if (function == NULL) {
    error(p, "failed to import %s", module_name);
    free(module_file);
    return;
  }

  function->name = NULL;

  push(p->vm, OBJ_VAL(function));
  b_obj_closure *closure = new_closure(p->vm, function);
  pop(p->vm);

  int import_constant = make_constant(p, OBJ_VAL(closure));
  push(p->vm, OBJ_VAL(closure));
  emit_byte_and_short(p, OP_CALL_IMPORT, import_constant);
  pop(p->vm);

  register_module__FILE__(p->vm, module);

  parse_specific_import(p, module_name, import_constant, was_renamed, false);
  free(module_file);
}

static void assert_statement(b_parser *p) {
  expression(p);
  if (match(p, COMMA_TOKEN)) {
    ignore_whitespace(p);
    expression(p);
  } else {
    emit_byte(p, OP_NIL);
  }

  emit_byte(p, OP_ASSERT);
  consume_statement_end(p);
}

static void catch_statement(b_parser *p) {
  int jump = emit_jump(p, OP_BEGIN_CATCH);

  ignore_whitespace(p);
  consume(p, LBRACE_TOKEN, "expected '{' after catch");
  begin_scope(p);
  block(p);
  end_scope(p);
  ignore_whitespace(p);

  patch_with_value(p, jump, current_blob(p)->count);
  emit_byte(p, OP_END_CATCH);

  if(match(p, AS_TOKEN)) {
    consume(p, IDENTIFIER_TOKEN, "missing exception variable name");
    created_variable(p, p->previous);
  } else {
    emit_byte(p, OP_POP);
  }
}

static void return_statement(b_parser *p) {
  p->is_returning = true;
  if (p->vm->compiler->type == TYPE_SCRIPT) {
    error(p, "cannot return from top-level code");
  }

  if (match(p, SEMICOLON_TOKEN) || match(p, NEWLINE_TOKEN)) {
    emit_return(p);
  } else {
    if (p->vm->compiler->type == TYPE_INITIALIZER) {
      error(p, "cannot return value from constructor");
    }

    expression(p);
    emit_byte(p, OP_RETURN);
    consume_statement_end(p);
  }
  p->is_returning = false;
}

static void while_statement(b_parser *p) {
  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // we'll be jumping back to right before the
  // expression after the loop body
  p->innermost_loop_start = current_blob(p)->count;
  p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

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

static void do_while_statement(b_parser *p) {
  int surrounding_loop_start = p->innermost_loop_start;
  int surrounding_scope_depth = p->innermost_loop_scope_depth;

  // we'll be jumping back to right before the
  // statements after the loop body
  p->innermost_loop_start = current_blob(p)->count;
  p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

  statement(p);

  consume(p, WHILE_TOKEN, "expecting 'while' statement");

  expression(p);

  int exit_jump = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);

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

  // discard local variables created in the loop
//  discard_local(p, p->innermost_loop_scope_depth);
  discard_locals(p, p->innermost_loop_scope_depth + 1);

  // go back to the top of the loop
  emit_loop(p, p->innermost_loop_start);
  consume_statement_end(p);
}

static void break_statement(b_parser *p) {
  if (p->innermost_loop_start == -1) {
    error(p, "'break' can only be used in a loop");
  }

  // discard local variables created in the loop
  /*for(int i = p->vm->compiler->local_count - 1; i >= 0 &&
    p->vm->compiler->locals[i].depth >= p->vm->compiler->scope_depth; i--) {
    if (p->vm->compiler->locals[i].is_captured) {
      emit_byte(p, OP_CLOSE_UP_VALUE);
    } else {
      emit_byte(p, OP_POP);
    }
  }*/
  discard_locals(p, p->innermost_loop_scope_depth + 1);
  emit_jump(p, OP_BREAK_PL);
  consume_statement_end(p);
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
      case WHEN_TOKEN:
      case ITER_TOKEN:
      case DO_TOKEN:
      case WHILE_TOKEN:
      case ECHO_TOKEN:
      case ASSERT_TOKEN:
      case TRY_TOKEN:
      case CATCH_TOKEN:
      case DIE_TOKEN:
      case RETURN_TOKEN:
      case STATIC_TOKEN:
      case SELF_TOKEN:
      case PARENT_TOKEN:
      case FINALLY_TOKEN:
      case IN_TOKEN:
      case IMPORT_TOKEN:
      case AS_TOKEN:
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
  } else if(match(p, LBRACE_TOKEN)) {
    if(!check(p, NEWLINE_TOKEN) && p->vm->compiler->scope_depth == 0) {
      expression_statement(p, false, true);
    } else {
      begin_scope(p);
      block(p);
      end_scope(p);
    }
  } else {
    statement(p);
  }

  ignore_whitespace(p);

  if (p->panic_mode)
    synchronize(p);

  ignore_whitespace(p);
}

static void statement(b_parser *p) {
  p->repl_can_echo = false;
  ignore_whitespace(p);

  if (match(p, ECHO_TOKEN)) {
    echo_statement(p);
  } else if (match(p, IF_TOKEN)) {
    if_statement(p);
  } else if (match(p, DO_TOKEN)) {
    do_while_statement(p);
  } else if (match(p, WHILE_TOKEN)) {
    while_statement(p);
  } else if (match(p, ITER_TOKEN)) {
    iter_statement(p);
  } else if (match(p, FOR_TOKEN)) {
    for_statement(p);
  } else if (match(p, USING_TOKEN)) {
    using_statement(p);
  } else if (match(p, CONTINUE_TOKEN)) {
    continue_statement(p);
  } else if (match(p, BREAK_TOKEN)) {
    break_statement(p);
  } else if (match(p, RETURN_TOKEN)) {
    return_statement(p);
  } else if (match(p, ASSERT_TOKEN)) {
    assert_statement(p);
  } else if (match(p, DIE_TOKEN)) {
    die_statement(p);
  } else if (match(p, LBRACE_TOKEN)) {
    begin_scope(p);
    block(p);
    end_scope(p);
  } else if (match(p, IMPORT_TOKEN)) {
    import_statement(p);
  } else if(match(p, CATCH_TOKEN)) {
    catch_statement(p);
  } else {
    expression_statement(p, false, false);
  }

  ignore_whitespace(p);
}

b_obj_func *compile(b_vm *vm, b_obj_module *module, const char *source) {
  b_scanner scanner;
  init_scanner(&scanner, source);

  b_parser parser;

  parser.vm = vm;
  parser.scanner = &scanner;

  parser.had_error = false;
  parser.panic_mode = false;
  parser.block_count = 0;
  parser.repl_can_echo = false;
  parser.is_returning = false;
  parser.is_trying = false;
  parser.innermost_loop_start = -1;
  parser.innermost_loop_scope_depth = 0;
  parser.current_class = NULL;
  parser.module = module;

  b_compiler compiler;
  init_compiler(&parser, &compiler, TYPE_SCRIPT);

  advance(&parser);
  ignore_whitespace(&parser);

  while (!match(&parser, EOF_TOKEN)) {
    declaration(&parser);
  }

  b_obj_func *function = end_compiler(&parser);

  return parser.had_error ? NULL : function;
}

void mark_compiler_roots(b_vm *vm) {
  b_compiler *compiler = vm->compiler;
  while (compiler != NULL) {
    mark_object(vm, (b_obj *) compiler->function);
    compiler = compiler->enclosing;
  }
}
