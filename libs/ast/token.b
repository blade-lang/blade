#!-- part of the ast module

# symbols
/**
 * newline token
 */
var NEWLINE = 0   # \n

/**
 * left parenthesis (`(`) token
 */
var LPAREN = 1   # (

/**
 * right parenthesis (`)`) token
 */
var RPAREN = 2   # )

/**
 * left bracket (`[`) token
 */
var LBRACKET = 3   # [

/**
 * right bracket (`]`) token
 */
var RBRACKET = 4   # ]

/**
 * left brace (`{`) token
 */
var LBRACE = 5   # {

/**
 * right brace (`}`) token
 */
var RBRACE = 6   # }

/**
 * semicolon (`;`) token
 */
var SEMICOLON = 7   # ;

/**
 * comma (`,`) token
 */
var COMMA = 8   # ,

/**
 * backslash (`\`) token
 */
var BACKSLASH = 9   # '\'

/**
 * not (`!`) token
 */
var BANG = 10   # !

/**
 * not equal (`!=`) token
 */
var BANG_EQ = 11   # !=

/**
 * colon (`:`) token
 */
var COLON = 12   # :

/**
 * at (`@`) token
 */
var AT = 13   # @

/**
 * dot (`.`) token
 */
var DOT = 14   # .

/**
 * range (`..`) token
 */
var RANGE = 15   # ..

/**
 * tridot (`...`) token
 */
var TRI_DOT = 16   # ...

/**
 * plus (`+`) token
 */
var PLUS = 17   # +

/**
 * plus equal (`+=`) token
 */
var PLUS_EQ = 18   # +=

/**
 * increment (`++`) token
 */
var INCREMENT = 19   # ++

/**
 * minus (`-`) token
 */
var MINUS = 20   # -

/**
 * minus equal (`-=`) token
 */
var MINUS_EQ = 21   # -=

/**
 * decrement (`--`) token
 */
var DECREMENT = 22   # --

/**
 * multiply (`*`) token
 */
var MULTIPLY = 23   # *

/**
 * multiply equal (`*=`) token
 */
var MULTIPLY_EQ = 24   # *=

/**
 * pow (`**`) token
 */
var POW = 24   # **

/**
 * pow equal (`**=`) token
 */
var POW_EQ = 26   # **=

/**
 * divide (`/`) token
 */
var DIVIDE = 27   # '/'

/**
 * divide equal (`/=`) token
 */
var DIVIDE_EQ = 28   # '/='

/**
 * floor division (`//`) token
 */
var FLOOR = 29   #'

/**
 * floor divide equal (`//=`) token
 */
var FLOOR_EQ = 30   #='

/**
 * assignment (`=`) token
 */
var EQUAL = 31   # =

/**
 * equality (`==`) token
 */
var EQUAL_EQ = 32   # ==

/**
 * less than (`<`) token
 */
var LESS = 33   # <

/**
 * less than or equal (`<=`) token
 */
var LESS_EQ = 34   # <=

/**
 * left shift (`<<`) token
 */
var LSHIFT = 35   # <<

/**
 * left shift equal (`<<=`) token
 */
var LSHIFT_EQ = 36   # <<=

/**
 * greater than (`>`) token
 */
var GREATER = 37   # >

/**
 * greather than or equal (`>=`) token
 */
var GREATER_EQ = 38   # >=

/**
 * right shift (`>>`) token
 */
var RSHIFT = 39   # >>

/**
 * right shift equal (`>>=`) token
 */
var RSHIFT_EQ = 40   # >>=

/**
 * modulous (`%`) token
 */
var PERCENT = 41   # %

/**
 * modulous equal (`%=`) token
 */
var PERCENT_EQ = 42   # %=

/**
 * ampersand (`&`) token
 */
var AMP = 43   # &

/**
 * and equal (`&=`) token
 */
var AMP_EQ = 44   # &=

/**
 * bar (`|`) token
 */
var BAR = 45   # |

/**
 * bar equal (`|=`) token
 */
var BAR_EQ = 46   # |=

/**
 * tilde/not (`~`) token
 */
var TILDE = 47   # ~

/**
 * tilde equal (`~=`) token
 */
var TILDE_EQ = 48   # ~=

/**
 * exclusive or (`^`) token
 */
var XOR = 49   # ^

/**
 * exclusive or equal (`^=`) token
 */
var XOR_EQ = 50   # ^=

/**
 * question (`?`) token
 */
var QUESTION = 51   # ?


# keywords
/**
 * and token
 */
var AND = 60

/**
 * as token
 */
var AS = 61

/**
 * assert token
 */
var ASSERT = 62

/**
 * break token
 */
var BREAK = 63

/**
 * catch token
 */
var CATCH = 64

/**
 * class token
 */
var CLASS = 65

/**
 * continue token
 */
var CONTINUE = 66

/**
 * def token
 */
var DEF = 67

/**
 * default token
 */
var DEFAULT = 68

/**
 * die token
 */
var DIE = 68

/**
 * do token
 */
var DO = 69

/**
 * echo token
 */
var ECHO = 70

/**
 * else token
 */
var ELSE = 71

/**
 * false token
 */
var FALSE = 72

/**
 * finally token
 */
var FINALLY = 73

/**
 * for token
 */
var FOR = 74

/**
 * if token
 */
var IF = 75

/**
 * import token
 */
var IMPORT = 76

/**
 * in token
 */
var IN = 77

/**
 * iter token
 */
var ITER = 78

/**
 * nil token
 */
var NIL = 79

/**
 * or token
 */
var OR = 80

/**
 * parent token
 */
var PARENT = 81

/**
 * return token
 */
var RETURN = 82

/**
 * self token
 */
var SELF = 83

/**
 * static token
 */
var STATIC = 84

/**
 * true token
 */
var TRUE = 85

/**
 * try token
 */
var TRY = 86

/**
 * using token
 */
var USING = 87

/**
 * var token
 */
var VAR = 88

/**
 * when token
 */
var WHEN = 89

/**
 * while token
 */
var WHILE = 90


# types token
/**
 * string literal token
 */
var LITERAL = 100

/**
 * regular number token
 */
var REG_NUMBER = 101   # regular numbers (inclusive of doubles)

/**
 * binary number token
 */
var BIN_NUMBER = 102   # binary numbers

/**
 * octal number token
 */
var OCT_NUMBER = 103   # octal numbers

/**
 * hexadecimal number token
 */
var HEX_NUMBER = 104   # hexadecimal numbers

/**
 * identifier token
 */
var IDENTIFIER = 105

/**
 * decorator token
 */
var DECORATOR = 106

/**
 * interpolation token
 */
var INTERPOLATION = 107


# comments
/**
 * comment token
 */
var COMMENT = 150

/**
 * doc block token
 */
var DOC = 160



#  * end of file
/**
 * eof token
 */
var EOF = 200



# errors
/**
 * error token
 */
var ERROR = 400

/**
 * empty token
 */
var EMPTY = 500


/**
 * Blade source code token
 * @serializable
 * @printable
 */
class Token {
  /**
   * Token(type: number, literal: string, line: number)
   * @constructor 
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
