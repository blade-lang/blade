#include "scanner.h"
#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_scanner(b_scanner *s, const char *source) {
  s->current = source;
  s->start = source;
  s->line = 1;
  s->interpolating_count = -1;
}

static bool is_at_end(b_scanner *s) { return *s->current == '\0'; }

static b_token make_token(b_scanner *s, b_tkn_type type) {
  b_token t;
  t.type = type;
  t.start = s->start;
  t.length = (int)(s->current - s->start);
  t.line = s->line;
  return t;
}

static b_token error_token(b_scanner *s, const char *message, int count, ...) {
  char *err = (char *)malloc(sizeof(char *) * count);

  va_list args;
  va_start(args, count);
  vsprintf(err, message, args);
  va_end(args);

  b_token t;
  t.type = ERROR_TOKEN;
  t.start = err;
  t.length = (int)strlen(err);
  t.line = s->line;
  return t;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_binary(char c) { return c == '0' || c == '1'; }

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_octal(char c) { return c >= '0' && c <= '7'; }

static bool is_hexadecimal(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

static char advance(b_scanner *s) {
  s->current++;
  if (s->current[-1] == '\n')
    s->line++;
  return s->current[-1];
}

static bool match(b_scanner *s, char expected) {
  if (is_at_end(s))
    return false;
  if (*s->current != expected)
    return false;

  s->current++;
  if (s->current[-1] == '\n')
    s->line++;
  return true;
}

static char current(b_scanner *s) { return *s->current; }

static char previous(b_scanner *s) { return s->current[-1]; }

static char next(b_scanner *s) {
  if (is_at_end(s))
    return '\0';
  return s->current[1];
}

b_token skip_block_comments(b_scanner *s) {
  int nesting = 1;
  while (nesting > 0) {
    if (is_at_end(s)) {
      return error_token(s, "unclosed block comment", 0);
    }

    // internal comment open
    if (current(s) == '/' && next(s) == '*') {
      advance(s);
      advance(s);
      nesting++;
      continue;
    }
    // comment close
    if (current(s) == '*' && next(s) == '/') {
      advance(s);
      advance(s);
      nesting--;
      continue;
    }

    // regular comment body
    advance(s);
  }

  return make_token(s, UNDEFINED_TOKEN);
}

b_token skip_whitespace(b_scanner *s) {
  for (;;) {
    char c = current(s);

    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance(s);
      break;

      /* case '\n': {
        s->line++;
        advance(s);
        break;
      } */

    case '#': { // single line comment
      while (current(s) != '\n' && !is_at_end(s))
        advance(s);
      break;
    }

    case '/':
      if (next(s) == '*') {
        advance(s);
        return skip_block_comments(s);
      } else {
        return make_token(s, UNDEFINED_TOKEN);
      }

    // exit as soon as we see a non-whitespace...
    default:
      return make_token(s, UNDEFINED_TOKEN);
    }
  }
  return make_token(s, UNDEFINED_TOKEN);
}

static b_token string(b_scanner *s, char quote) {
  while (
      ((previous(s) == '\\' && current(s) == quote) || current(s) != quote) &&
      !is_at_end(s)) {
    if (current(s) == '$' && next(s) == '{' && previous(s) != '\\') { // interpolation started
      if(s->interpolating_count - 1 < MAX_INTERPOLATION_NESTING) {
        s->interpolating[s->interpolating_count++] = quote;
        s->current++;
        b_token tkn = make_token(s, INTERPOLATION_TOKEN);
        s->current++;
        return tkn;
      }

      return error_token(s, "maxmimum interpolation nesting exceeded", 0);
    }
    advance(s);
  }

  if (is_at_end(s))
    return error_token(s, "unterminated string (opening quote not matched)", 0);

  match(s, quote); // the closing quote
  return make_token(s, LITERAL_TOKEN);
}

static b_token number(b_scanner *s) {
  // handle binary, octals and hexadecimals
  if (previous(s) == '0') {
    if (match(s, 'b')) { // binary number
      while (is_binary(current(s)))
        advance(s);

      return make_token(s, BIN_NUMBER_TOKEN);
    } else if (match(s, 'c')) {
      while (is_octal(current(s)))
        advance(s);

      return make_token(s, OCT_NUMBER_TOKEN);
    } else if (match(s, 'x')) {
      while (is_hexadecimal(current(s)))
        advance(s);

      return make_token(s, HEX_NUMBER_TOKEN);
    }
  }

  while (is_digit(current(s)))
    advance(s);

  // dots(.) are only valid here when followed by a digit
  if (current(s) == '.' && is_digit(next(s))) {
    advance(s);

    while (is_digit(current(s)))
      advance(s);
  }

  return make_token(s, REG_NUMBER_TOKEN);
}

static b_tkn_type check_keyword(b_scanner *s, int start, int length,
                                const char *rest, b_tkn_type type) {
  if (s->current - s->start == start + length &&
      memcmp(s->start + start, rest, length) == 0) {
    return type;
  }
  return IDENTIFIER_TOKEN;
}

static b_tkn_type identifier_type(b_scanner *s) {
  switch (s->start[0]) {
  case 'a':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'n':
        return check_keyword(s, 2, 1, "d", AND_TOKEN);
      case 's':
        if (s->current - s->start > 2) {
          return check_keyword(s, 2, 4, "sert", ASSERT_TOKEN);
        } else {
          return check_keyword(s, 2, 0, "", AS_TOKEN);
        }
      }
    }
  case 'b':
    return check_keyword(s, 1, 4, "reak", BREAK_TOKEN);
  case 'c':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'a':
        return check_keyword(s, 2, 3, "tch", CATCH_TOKEN);
      case 'l':
        return check_keyword(s, 2, 3, "ass", CLASS_TOKEN);
      case 'o':
        return check_keyword(s, 2, 6, "ntinue", CONTINUE_TOKEN);
      }
    }
    break;
  case 'd':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'e':
        if (s->current - s->start > 2) {
          switch (s->start[2]) {
          case 'f':
            if (s->current - s->start > 3)
              return check_keyword(s, 3, 4, "ault", DEFAULT_TOKEN);
            else
              return check_keyword(s, 3, 0, "", DEF_TOKEN);
            break;
          }
        }
      case 'i':
        return check_keyword(s, 2, 1, "e", DIE_TOKEN);
      }
    }
  case 'e':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'c':
        return check_keyword(s, 2, 2, "ho", ECHO_TOKEN);
      case 'l':
        return check_keyword(s, 2, 2, "se", ELSE_TOKEN);
      case 'm':
        return check_keyword(s, 2, 3, "pty", EMPTY_TOKEN);
      }
    }
  case 'f':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'a':
        return check_keyword(s, 2, 3, "lse", FALSE_TOKEN);
      case 'o':
        return check_keyword(s, 2, 1, "r", FOR_TOKEN);
      }
    }
  case 'i':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'f':
        return check_keyword(s, 2, 0, "", IF_TOKEN);
      case 'm':
        return check_keyword(s, 2, 4, "port", IMPORT_TOKEN);
      case 'n':
        return check_keyword(s, 2, 0, "", IN_TOKEN);
      case 't':
        return check_keyword(s, 2, 2, "er", ITER_TOKEN);
      }
    }
  case 'n':
    return check_keyword(s, 1, 2, "il", NIL_TOKEN);
  case 'o':
    return check_keyword(s, 1, 1, "r", OR_TOKEN);
  case 'p':
    return check_keyword(s, 1, 5, "arent", PARENT_TOKEN);
  case 'r':
    return check_keyword(s, 1, 5, "eturn", RETURN_TOKEN);
  case 's':
    if (s->current - s->start > 1) {
      switch (s->start[1]) {
      case 'e':
        return check_keyword(s, 2, 2, "lf", SELF_TOKEN);
      case 't':
        return check_keyword(s, 2, 4, "atic", STATIC_TOKEN);
      }
    }
  case 't':
    if (s->current - s->start > 2 && s->start[1] == 'r') {
      switch (s->start[2]) {
      case 'u':
        return check_keyword(s, 3, 1, "e", TRUE_TOKEN);
      case 'y':
        return check_keyword(s, 3, 0, "", TRY_TOKEN);
      }
    }
  case 'u': {
    return check_keyword(s, 1, 4, "sing", USING_TOKEN);
  }
  case 'v':
    return check_keyword(s, 1, 2, "ar", VAR_TOKEN);
  case 'w':
    if (s->current - s->start > 2 && s->start[1] == 'h') {
      switch (s->start[2]) {
      case 'i':
        return check_keyword(s, 3, 2, "le", WHILE_TOKEN);
      case 'e':
        return check_keyword(s, 3, 1, "n", WHEN_TOKEN);
      }
    }
  }
  return IDENTIFIER_TOKEN;
}

static b_token identifier(b_scanner *s) {
  while (is_alpha(current(s)) || is_digit(current(s)))
    advance(s);
  return make_token(s, identifier_type(s));
}

b_token scan_token(b_scanner *s) {
  b_token tk = skip_whitespace(s);
  if (tk.type != UNDEFINED_TOKEN) {
    return tk;
  }

  s->start = s->current;

  if (is_at_end(s))
    return make_token(s, EOF_TOKEN);

  char c = advance(s);

  if (is_digit(c))
    return number(s);
  else if (is_alpha(c))
    return identifier(s);

  switch (c) {
  case '(':
    return make_token(s, LPAREN_TOKEN);
  case ')': 
    return make_token(s, RPAREN_TOKEN);
  case '[':
    return make_token(s, LBRACKET_TOKEN);
  case ']':
    return make_token(s, RBRACKET_TOKEN);
  case '{':
    return make_token(s, LBRACE_TOKEN);
  case '}':
    if (s->interpolating_count > -1) {
      s->interpolating_count--;
      return string(s, s->interpolating[s->interpolating_count]);
    }
    return make_token(s, RBRACE_TOKEN);
  case ';':
    return make_token(s, SEMICOLON_TOKEN);
  case '\\':
    return make_token(s, BACKSLASH_TOKEN);
  case ':':
    return make_token(s, COLON_TOKEN);
  case ',':
    return make_token(s, COMMA_TOKEN);
  case '@':
    return make_token(s, AT_TOKEN);
  case '!':
    return make_token(s, match(s, '=') ? BANG_EQ_TOKEN : BANG_TOKEN);
  case '.':
    if (match(s, '.')) {
      return make_token(s, match(s, '.') ? TRIDOT_TOKEN : RANGE_TOKEN);
    }
    return make_token(s, DOT_TOKEN);
  case '+': {
    if (match(s, '+'))
      return make_token(s, INCREMENT_TOKEN);
    if (match(s, '='))
      return make_token(s, PLUS_EQ_TOKEN);
    else
      return make_token(s, PLUS_TOKEN);
  }
  case '-': {
    if (match(s, '-'))
      return make_token(s, DECREMENT_TOKEN);
    if (match(s, '='))
      return make_token(s, MINUS_EQ_TOKEN);
    else
      return make_token(s, MINUS_TOKEN);
  }
  case '*':
    if (match(s, '*')) {
      return make_token(s, match(s, '=') ? POW_EQ_TOKEN : POW_TOKEN);
    } else {
      return make_token(s, match(s, '=') ? MULTIPLY_EQ_TOKEN : MULTIPLY_TOKEN);
    }
  case '/':
    if (match(s, '/')) {
      return make_token(s, match(s, '=') ? FLOOR_EQ_TOKEN : FLOOR_TOKEN);
    } else {
      return make_token(s, match(s, '=') ? DIVIDE_EQ_TOKEN : DIVIDE_TOKEN);
    }
  case '=':
    return make_token(s, match(s, '=') ? EQUAL_EQ_TOKEN : EQUAL_TOKEN);
  case '<':
    if (match(s, '<')) {
      return make_token(s, match(s, '=') ? LSHIFT_EQ_TOKEN : LSHIFT_TOKEN);
    } else {
      return make_token(s, match(s, '=') ? LESS_EQ_TOKEN : LESS_TOKEN);
    }
  case '>':
    if (match(s, '>')) {
      return make_token(s, match(s, '=') ? RSHIFT_EQ_TOKEN : RSHIFT_TOKEN);
    } else {
      return make_token(s, match(s, '=') ? GREATER_EQ_TOKEN : GREATER_TOKEN);
    }
  case '%':
    return make_token(s, match(s, '=') ? PERCENT_EQ_TOKEN : PERCENT_TOKEN);
  case '&':
    return make_token(s, match(s, '=') ? AMP_EQ_TOKEN : AMP_TOKEN);
  case '|':
    return make_token(s, match(s, '=') ? BAR_EQ_TOKEN : BAR_TOKEN);
  case '~':
    return make_token(s, match(s, '=') ? TILDE_EQ_TOKEN : TILDE_TOKEN);
  case '^':
    return make_token(s, match(s, '=') ? XOR_EQ_TOKEN : XOR_TOKEN);

  // newline
  case '\n':
    return make_token(s, NEWLINE_TOKEN);

  case '"':
    return string(s, '"');
  case '\'':
    return string(s, '\'');

  // --- DO NOT MOVE ABOVE OR BELOW THE DEFAULT CASE ---
  // fall-through tokens... this tokens are only valid
  // when the carry another token with them...
  // be careful not to add break after them so that they may use the default
  // case.
  case '?':
    if (match(s, '?')) {
      return make_token(s, CDEFAULT_TOKEN);
    }

  default:
    break;
  }

  return error_token(s, "unexpected character %c", 1, c);
}