#!-- part of the ast module

import .token { * }
import .expr { * }
import .stmt { * }
import .decl { * }

/**
 * @class ParseException
 */
class ParseException < Exception {
  /**
   * @constructor ParseException
   * ParseException(token: Token, message: string)
   */
  ParseException(token, message) {
    parent('Error at ${token.literal}: ${message}')
  }
}


/**
 * @class Parser
 * 
 * Parses raw Blade tokens and produces an Abstract Syntax Tree
 */
class Parser {

  /**
   * a pointer to the next token waiting to be parsed
   */
  var _current = 0

  var tokens = []

  /**
   * @constructor Parser
   * Parser(tokens: []Token)
   */
  Parser(tokens) {
    # set instance variable token
    self.tokens = tokens
  }

  /**
   * checks to see if the current token has any of the given types
   */
  _match(...) {
    for t in __args__ {
      if self._check(t) {
        self._advance()
        return true
      }
    }

    return false
  }

  /**
   * returns true if the current token is of the given type
   * otherwise returns false
   */
  _check(type) {
    if self._is_at_end() return false
    return self._peek().type == type
  }

  /**
   * consumes the current token and returns it
   */
  _advance() {
    if !self._is_at_end() self._current++
    return self._previous()
  }

  /**
   * checks if we’ve run out of tokens to parse
   */
  _is_at_end() {
    return self._peek().type == EOF
  }

  /**
   * returns the current token we have yet to consume
   */
  _peek() {
    return self.tokens[self._current]
  }

  /**
   * returns the most recently consumed token
   */
  _previous() {
    return self.tokens[self._current - 1]
  }

  _consume(type, message) {
    if self._check(type) return self._advance()
    die ParseException(self._peek(), message)
  }

  ### EXPRESSIONS START

  _grouping() {
    var expr = self._expression()
    self._consume(RPAREN, "Expected ')' after expression")
    return Grouping(expr)
  }

  _primary() {
    if self._match(FALSE) return Literal(false)
    if self._match(TRUE) return Literal(true)
    if self._match(NIL) return Literal(nil)

    if self._match(BIN_NUMBER, HEX_NUMBER, OCT_NUMBER, REG_NUMBER, LITERAL)
      return Literal(self._previous().literal)

    if self._match(LPAREN) return self._grouping()
  }

  /**
   * factor expressions
   */
  _unary() {
    if self._match(BANG, MINUS, TILDE) {
      var op = self._previous()
      var right = self._primary()
      return Unary(op, right)
    }

    return self._primary()
  }

  /**
   * factor expressions
   */
  _factor() {
    var expr = self._unary()

    while self._match(MULTIPLY, DIVIDE, PERCENT, POW, FLOOR) {
      var op = self._previous()
      var right = self._unary()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * term expressions
   */
  _term() {
    var expr = self._factor()

    while self._match(PLUS, MINUS) {
      var op = self._previous()
      var right = self._factor()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * range expressions
   */
  _range() {
    var expr = self._term()

    while self._match(RANGE) {
      var op = RANGE
      var right = self._term()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * shift expressions
   */
  _shift() {
    var expr = self._range()

    while self._match(LSHIFT, RSHIFT) {
      var op = self._previous()
      var right = self._range()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * bit and (&) expressions
   */
  _bit_and() {
    var expr = self._shift()

    while self._match(AMP) {
      var op = AMP
      var right = self._shift()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * bit xor (^) expressions
   */
  _bit_xor() {
    var expr = self._bit_and()

    while self._match(XOR) {
      var op = XOR
      var right = self._bit_and()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * bit or (|) expressions
   */
  _bit_or() {
    var expr = self._bit_xor()

    while self._match(BAR) {
      var op = BAR
      var right = self._bit_xor()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * comparison expressions
   */
  _comparison() {
    var expr = self._bit_or()

    while self._match(GREATER, GREATER_EQ, LESS, LESS_EQ) {
      var op = self._previous()
      var right = self._bit_or()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * parses an equality expression
   */
  _equality() {
    var expr = self._comparison()

    while self._match(BANG_EQ, EQUAL_EQ) {
      var op = self._previous()
      var right = self._comparison()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * parses and expression
   */
  _and() {
    var expr = self._equality()

    while self._match(AND) {
      var op = AND
      var right = self._equality()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * parses or expression
   */
  _or() {
    var expr = self._and()

    while self._match(OR) {
      var op = OR
      var right = self._and()
      expr = Binary(expr, op, right)
    }

    return expr
  }

  /**
   * parses conditionals
   */
  _conditional() {
    var expr = self._or()

    if self._match(QUESTION) {
      var truth = self._or()
      self._consume(COLON, ': expected in tenary operation')
      expr = Condition(expr, truth, self._or())
    }

    return expr
  }

  /**
   * parses an expression
   */
  _expression() {
    return self._conditional()
  }

  ### EXPRESSIONS END

  parse() {
    return self._expression()
  }
}