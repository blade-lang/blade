#!-- part of the ast module

import .token { * }

/**
 * Blade source code scanner.
 * 
 * @printable
 */
class Scanner {
  /**
   * Reports if an error was encountered in the scaner.
   * 
   * @type bool
   * @readonly
   */
  var has_error = false

  /**
   * The string to being scanned.
   * 
   * @type string
   * @readonly
   */
  var source

  /**
   * a list of tokens processed by this scanner
   */
  var _tokens = []

  /**
   * scanner location indicators
   */
  var _line = 1
  var _current = 0
  var _start = 0


  /**
   * interpolation tracking
   */
  var _max_interpolation_nest = 8

  /**
   * a keyword to token map
   */
  var _keywords = {
    'and': AND,
    'as': AS,
    'assert': ASSERT,
    'break': BREAK,
    'catch': CATCH,
    'class': CLASS,
    'continue': CONTINUE,
    'def': DEF,
    'default': DEFAULT,
    'die': DIE,
    'do': DO,
    'echo': ECHO,
    'else': ELSE,
    'false': FALSE,
    'finally': FINALLY,
    'for': FOR,
    'if': IF,
    'import': IMPORT,
    'in': IN,
    'iter': ITER,
    'nil': NIL,
    'or': OR,
    'parent': PARENT,
    'return': RETURN,
    'self': SELF,
    'static': STATIC,
    'true': TRUE,
    'try': TRY,
    'using': USING,
    'var': VAR,
    'when': WHEN,
    'while': WHILE
  }

  /**
   * @param string source
   * @constructor 
   */
  Scanner(source, file) {
    if !is_string(source)
      die Exception('Blade source code expected')
    
    # to track the quote that started an interpolation
    self._interpolating = []
    self._file = file
    
    self.source = source
  }

  /**
   * checks to see if we have reached the end of the source
   */
  _is_at_end() {
    return self._current >= self.source.length()
  }

  _is_digit(c) {
    if !c return false
    var _ = ord(c)
    return _ >= 48 and _ <= 57  # 0 - 9
  }

  _is_alpha(c) {
    if !c return false
    var _ = ord(c)
    return (_ >= 97 and _ <= 122) or  # a - z
          (_ >= 65 and _ <= 90) or _ == 95  # A - Z or _
  }

  _is_alphanumeric(c) {
    return self._is_alpha(c) or self._is_digit(c)
  }

  _is_binary(c) {
    if !c return false
    return c == '0' or c == '1'
  }

  _is_octal(c) {
    if !c return false
    var _ = ord(c)
    return _ >= 48 and _ <= 55  # 0 - 7
  }

  _is_hexadecimal(c) {
    if !c return false
    var _ = ord(c)
    return self._is_digit(c) or  # 0 - 9
        (_ >= 97 and _ <= 102) or # a - f
        (_ >= 65 and _ <= 70)   # A - F
  }

  /**
   * returns the current token while moving the pointer forward
   */
  _advance() {
    var val = self.source[self._current]
    self._current++
    if val == '\n' self._line++
    return val
  }

  /**
   * advances if the current character is the given character
   */
  _match(c) {
    if self._is_at_end() return false
    if self.source[self._current] != c return false
    self._current++
    return true
  }

  /**
   * returns the character at the next position without
   * consuming the current one.
   */
  _next() {
    if self._current + 1 >= self.source.length() return '\0'
    return self.source[self._current + 1]
  }

  /**
   * returns the character at the next position without
   * consuming the current one.
   */
  _previous() {
    if self._current == 0 return '\0'
    return self.source[self._current - 1]
  }

  /**
   * returns the character at the current position without
   * consuming it.
   */
  _peek() {
    if self._is_at_end() return '\0'
    return self.source[self._current]
  }

  /**
   * _add_token(type: number [, literal: string])
   * adds a new token to the list of tokens
   */
  _add_token(type, literal) {
    if !literal {
      literal = self.source[self._start, self._current].trim()
    }
    self._tokens.append(Token(type, literal, self._line, self._file))
  }

  /**
   * skips block comments
   */
  _skip_block_comment() {
    var nesting = 1
    while nesting > 0 {
      if self._is_at_end() return

      # internal comment open
      if self._peek() == '/' and self._next() == '*' {
        self._advance()
        self._advance()
        nesting++
      }

      # comment close
      else if self._peek() == '*' and self._next() == '/' {
        self._advance()
        self._advance()
        nesting--
      }

      else self._advance()
    }
  }

  /**
   * skips whitespace and comments
   */
  _skip_whitespace() {
    while true {
      var c = self._peek()

      using c {

        # whitespace
        when ' ' self._advance()
        when '\r' self._advance()
        when '\t' self._advance()

        # single line comment
        when '#' {
          self._advance()
          while self._peek() != '\n' and !self._is_at_end()
            self._advance()
          self._add_token(COMMENT)
          self._advance()
          self._start = self._current
        }

        default return
      }
    }
  }

  /**
   * parses a decorator name
   */
  _decorator() {
    while self._is_alpha(self._peek()) or self._is_digit(self._peek())
      self._advance()
    self._add_token(DECORATOR)
  }

  /**
   * parses a string surrounded by the quote c.
   */
  _string(c) {
    while self._peek() != c and !self._is_at_end() {
      if self._peek() == '$' and self._next() == '{' and
        self._previous() != '\\' {  # interpolation started

        if self._interpolating.length() < self._max_interpolation_nest {
          self._interpolating.append(c)
          self._current++
          self._add_token(INTERPOLATION)
          self._current++
          return
        }

        die Exception('maximum interpolation nesting exceeded')
      }

      if self._peek() == '\\' and (self._next() == c or self._next() == '\\')
        self._advance()
      
      self._advance()
    }

    if self._is_at_end() 
      die Exception('unterminated string on line ${self._line}')

    self._match(c)
    self._add_token(LITERAL, self.source[self._start + 1, self._current - 1])
  }

  /**
   * parses a valid Blade number
   */
  _number() {
    if self._previous() == '0' {
      if self._match('b') {   # binary number
        while self._is_binary(self._peek())
          self._advance()

        self._add_token(BIN_NUMBER)
        return
      } else if self._match('c') {  # octal number
        while self._is_octal(self._peek())
          self._advance()

        self._add_token(OCT_NUMBER)
        return
      } else if self._match('x') {  # hex number
        while self._is_hexadecimal(self._peek())
          self._advance()

        self._add_token(HEX_NUMBER)
        return
      }
    }

    while self._is_digit(self._peek())
      self._advance()

    if self._peek() == '.' and self._is_digit(self._next()) {
      self._advance()

      while self._is_digit(self._peek())
        self._advance()

      # E or e are only valid when followed by a didigt and 
      # occurring after a dot.
      if (self._peek() == 'e' or self._peek() == 'E') and
          (self._next() == '+' or self._next() == '-') {
        
        self._advance()
        self._advance()

        while self._is_digit(self._peek())
          self._advance()
      }
    }

    self._add_token(REG_NUMBER)
  }

  /**
   * scans identifiers and keywords
   */
  _identifier() {
    while self._is_alphanumeric(self._peek())
      self._advance()

    var text = self.source[self._start, self._current].trim()
    self._add_token(self._keywords.get(text, IDENTIFIER), text)
  }

  /**
   * the private scanning helper
   */
  _scan() {
    if self._is_at_end() {
      return
    }

    var c = self._advance()

    using c {
      when '(' self._add_token(LPAREN)
      when ')' self._add_token(RPAREN)
      when '[' self._add_token(LBRACKET)
      when ']' self._add_token(RBRACKET)
      when '{' self._add_token(LBRACE)
      when '}' {
        if self._interpolating.length() > 0 {
          self._string(self._interpolating.pop())
        } else {
          self._add_token(RBRACE)
        }
      }
      when ',' self._add_token(COMMA)
      when ';' self._add_token(SEMICOLON)
      when '@' {
        if !self._is_alpha(self._peek()) {
          self._add_token(AT)
        } else {
          self._decorator()
        }
      }
      when '.' {
        if self._match('.') {
          self._add_token(self._match('.') ? TRI_DOT : RANGE)
        } else {
          self._add_token(DOT)
        }
      }
      when '-' {
        if self._match('-') {
          self._add_token(DECREMENT)
        } else if self._match('=') {
          self._add_token(MINUS_EQ)
        } else {
          self._add_token(MINUS)
        }
      }
      when '+' {
        if self._match('+') {
          self._add_token(INCREMENT)
        } else if self._match('=') {
          self._add_token(PLUS_EQ)
        } else {
          self._add_token(PLUS)
        }
      }
      when '*' {
        if self._match('*') {
          self._add_token(self._match('=') ? POW_EQ : POW)
        } else {
          self._add_token(self._match('=') ? MULTIPLY_EQ : MULTIPLY)
        }
      }
      when '/' {
        if self._match('/') {
          self._add_token(self._match('=') ? FLOOR_EQ : FLOOR)
        } else if self._match('*') {
          if self._match('*') {
            self._advance()
            self._skip_block_comment()
            self._add_token(DOC)
            self._start = self._current
          } else {
            self._advance()
            self._skip_block_comment()
            self._add_token(COMMENT)
            self._start = self._current
          }
        } else {
          self._add_token(self._match('=') ? DIVIDE_EQ : DIVIDE)
        }
      }
      when '\\' self._add_token(BACKSLASH)
      when ':' self._add_token(COLON)
      when '<' {
        if self._match('<') {
          self._add_token(self._match('=') ? LSHIFT_EQ : LSHIFT)
        } else {
          self._add_token(self._match('=') ? LESS_EQ : LESS)
        }
      }
      when '>' {
        if self._match('>') {
          self._add_token(self._match('=') ? RSHIFT_EQ : RSHIFT)
        } else {
          self._add_token(self._match('=') ? GREATER_EQ : GREATER)
        }
      }
      when '!' self._add_token(self._match('=') ? BANG_EQ : BANG)
      when '=' self._add_token(self._match('=') ? EQUAL_EQ : EQUAL)
      when '%' self._add_token(self._match('=') ? PERCENT_EQ : PERCENT)
      when '&' self._add_token(self._match('=') ? AMP_EQ : AMP)
      when '|' self._add_token(self._match('=') ? BAR_EQ : BAR)
      when '^' self._add_token(self._match('=') ? XOR_EQ : XOR)
      when '~' self._add_token(self._match('=') ? TILDE_EQ : TILDE)
      when '?' self._add_token(QUESTION)

      # newline token
      when '\n' self._add_token(NEWLINE)

      when "'" self._string("'")
      when '"' self._string('"')

      default {
        if self._is_digit(c) {
          self._number()
        } else if self._is_alpha(c) {
          self._identifier()
        } else {
          die Exception("Unexpected character '${c}'")
        }
      }
    }
  }

  /**
   * Scans the source and returns a list of tokens.
   * 
   * @returns list[Token]
   */
  scan() {

    while !self._is_at_end() {
      self._skip_whitespace()
      
      # scan tokens here...
      self._start = self._current
      self._scan()
    }

    self._tokens.append(Token(EOF, 'end of file', self._line, self._file))
    return self._tokens
  }

  @to_string() {
    return '<ast::Scanner>'
  }
}

