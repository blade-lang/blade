#!-- part of the ast module

/**
 * symbols
 */
var NEWLINE = 0   # \n
var LPAREN = 1   # (
var RPAREN = 2   # )
var LBRACKET = 3   # [
var RBRACKET = 4   # ]
var LBRACE = 5   # {
var RBRACE = 6   # }
var SEMICOLON = 7   # ;
var COMMA = 8   # ,
var BACKSLASH = 9   # '\'
var BANG = 10   # !
var BANG_EQ = 11   # !=
var COLON = 12   # :
var AT = 13   # @
var DOT = 14   # .
var RANGE = 15   # ..
var TRI_DOT = 16   # ...
var PLUS = 17   # +
var PLUS_EQ = 18   # +=
var INCREMENT = 19   # ++
var MINUS = 20   # -
var MINUS_EQ = 21   # -=
var DECREMENT = 22   # --
var MULTIPLY = 23   # *
var MULTIPLY_EQ = 24   # *=
var POW = 24   # **
var POW_EQ = 26   # **=
var DIVIDE = 27   # '/'
var DIVIDE_EQ = 28   # '/='
var FLOOR = 29   #'
var FLOOR_EQ = 30   #='
var EQUAL = 31   # =
var EQUAL_EQ = 32   # ==
var LESS = 33   # <
var LESS_EQ = 34   # <=
var LSHIFT = 35   # <<
var LSHIFT_EQ = 36   # <<=
var GREATER = 37   # >
var GREATER_EQ = 38   # >=
var RSHIFT = 39   # >>
var RSHIFT_EQ = 40   # >>=
var PERCENT = 41   # %
var PERCENT_EQ = 42   # %=
var AMP = 43   # &
var AMP_EQ = 44   # &=
var BAR = 45   # |
var BAR_EQ = 46   # |=
var TILDE = 47   # ~
var TILDE_EQ = 48   # ~=
var XOR = 49   # ^
var XOR_EQ = 50   # ^=
var QUESTION = 51   # ??

/**
 * keywords
 */
var AND = 60
var AS = 61
var ASSERT = 62
var BREAK = 63
var CATCH = 64
var CLASS = 65
var CONTINUE = 66
var DEF = 67
var DEFAULT = 68
var DIE = 68
var ECHO = 69
var ELSE = 70
var FALSE = 71
var FINALLY = 72
var FOR = 73
var IF = 74
var IMPORT = 75
var IN = 76
var ITER = 77
var NIL = 78
var OR = 79
var PARENT = 80
var RETURN = 81
var SELF = 82
var STATIC = 83
var TRUE = 84
var TRY = 85
var USING = 86
var VAR = 87
var WHEN = 88
var WHILE = 89

/**
 * types token
 */
var LITERAL = 100
var REG_NUMBER = 101   # regular numbers (inclusive of doubles)
var BIN_NUMBER = 102   # binary numbers
var OCT_NUMBER = 103   # octal numbers
var HEX_NUMBER = 104   # hexadecimal numbers
var IDENTIFIER = 105
var DECORATOR = 106
var INTERPOLATION = 107

/**
 * comments
 */
var COMMENT = 150
var DOC = 160

/**
 * end of file
 */
var EOF = 200


/**
 * error
 */
var ERROR = 400
var EMPTY = 500

/**
 * @class Token
 * 
 * Blade source code token
 */
class Token {
  /**
   * @constructor Token
   * Token(type: number, literal: string, line: number)
   */
  Token(type, literal, line) {
    self.type = type
    self.literal = literal
    self.line = line
  }

  @to_string() {
    return "<ast::Token type=${self.type} literal='${self.literal}' line=${self.line}>"
  }

  @to_json() {
    return {
      type: self.type,
      literal: self.literal,
      line: self.line
    }
  }
}