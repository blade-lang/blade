#ifndef bird_scanner_h
#define bird_scanner_h

#include "common.h"

typedef enum {
  // symbols
  NEWLINE_TOKEN,     // \n
  LPAREN_TOKEN,      // (
  RPAREN_TOKEN,      // )
  LBRACKET_TOKEN,    // [
  RBRACKET_TOKEN,    // ]
  LBRACE_TOKEN,      // {
  RBRACE_TOKEN,      // }
  SEMICOLON_TOKEN,   // ;
  COMMA_TOKEN,       // ,
  BACKSLASH_TOKEN,   // '\'
  BANG_TOKEN,        // !
  BANG_EQ_TOKEN,     // !=
  COLON_TOKEN,       // :
  AT_TOKEN,          // @
  DOT_TOKEN,         // .
  RANGE_TOKEN,       // ..
  TRIDOT_TOKEN,      // ...
  PLUS_TOKEN,        // +
  PLUS_EQ_TOKEN,     // +=
  INCREMENT_TOKEN,   // ++
  MINUS_TOKEN,       // -
  MINUS_EQ_TOKEN,    // -=
  DECREMENT_TOKEN,   // --
  MULTIPLY_TOKEN,    // *
  MULTIPLY_EQ_TOKEN, // *=
  POW_TOKEN,         // **
  POW_EQ_TOKEN,      // **=
  DIVIDE_TOKEN,      // '/'
  DIVIDE_EQ_TOKEN,   // '/='
  FLOOR_TOKEN,       // '//'
  FLOOR_EQ_TOKEN,    // '//='
  EQUAL_TOKEN,       // =
  EQUAL_EQ_TOKEN,    // ==
  LESS_TOKEN,        // <
  LESS_EQ_TOKEN,     // <=
  LSHIFT_TOKEN,      // <<
  LSHIFT_EQ_TOKEN,   // <<=
  GREATER_TOKEN,     // >
  GREATER_EQ_TOKEN,  // >=
  RSHIFT_TOKEN,      // >>
  RSHIFT_EQ_TOKEN,   // >>=
  PERCENT_TOKEN,     // %
  PERCENT_EQ_TOKEN,  // %=
  AMP_TOKEN,         // &
  AMP_EQ_TOKEN,      // &=
  BAR_TOKEN,         // |
  BAR_EQ_TOKEN,      // |=
  TILDE_TOKEN,       // ~
  TILDE_EQ_TOKEN,    // ~=
  XOR_TOKEN,         // ^
  XOR_EQ_TOKEN,      // ^=
  CDEFAULT_TOKEN,    // ??

  // keywords
  AND_TOKEN,
  AS_TOKEN,
  ASSERT_TOKEN,
  BREAK_TOKEN,
  CLASS_TOKEN,
  CONTINUE_TOKEN,
  DEF_TOKEN,
  DEFAULT_TOKEN,
  DIE_TOKEN,
  ECHO_TOKEN,
  ELSE_TOKEN,
  FALSE_TOKEN,
  FOR_TOKEN,
  IF_TOKEN,
  IMPORT_TOKEN,
  IN_TOKEN,
  ITER_TOKEN,
  VAR_TOKEN,
  NIL_TOKEN,
  OR_TOKEN,
  PARENT_TOKEN,
  RETURN_TOKEN,
  SELF_TOKEN,
  STATIC_TOKEN,
  TRUE_TOKEN,
  USING_TOKEN,
  WHEN_TOKEN,
  WHILE_TOKEN,
  TRY_TOKEN,
  CATCH_TOKEN,

  // types token
  LITERAL_TOKEN,
  REG_NUMBER_TOKEN, // regular numbers (inclusive of doubles)
  BIN_NUMBER_TOKEN, // binary numbers
  OCT_NUMBER_TOKEN, // octal numbers
  HEX_NUMBER_TOKEN, // hexadecimal numbers
  IDENTIFIER_TOKEN,
  INTERPOLATION_TOKEN,
  EOF_TOKEN,

  // error
  ERROR_TOKEN,
  EMPTY_TOKEN,
  UNDEFINED_TOKEN,
} b_tkn_type;

typedef struct {
  b_tkn_type type;
  const char *start;
  int length;
  int line;
} b_token;

typedef struct {
  const char *start;
  const char *current;
  int line;
  char interpolating[MAX_INTERPOLATION_NESTING];
  int interpolating_count;
} b_scanner;

void init_scanner(b_scanner *s, const char *source);
b_token scan_token(b_scanner *s);

#endif