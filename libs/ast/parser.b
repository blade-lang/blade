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
    parent('Error at ${token.literal} on line ${token.line}: ${message}')
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

  /**
   * the nested scope depth
   */
  var _block_count = 0

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
    if self._is_at_end() and type != EOF return false
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

  /**
   * continues only if the next token is of the given type.
   * Otherwise, it reports the given message as a ParseException
   */
  _consume(type, message) {
    if self._check(type) return self._advance()
    die ParseException(self._peek(), message)
  }

  _end_statement() {
    if self._match(EOF) or self._is_at_end() return

    if self._block_count > 0 and self._check(RBRACE) return

    if self._match(SEMICOLON) {
      while self._match(NEWLINE, SEMICOLON) {}
      return
    }

    self._consume(NEWLINE, 'end of statement expected')

    while self._match(NEWLINE, SEMICOLON) {}
  }

  /**
   * ignores consecutive newlines
   */
  _ignore_newline() {
    while self._match(NEWLINE) {}
  }

  ### EXPRESSIONS START

  /**
   * grouped expressions
   */
  _grouping() {
    var expr = self._expression()
    self._consume(RPAREN, "Expected ')' after expression")
    return GroupingExpr(expr)
  }

  /**
   * primary expressions
   */
  _primary() {
    if self._match(FALSE) return LiteralExpr(false)
    if self._match(TRUE) return LiteralExpr(true)
    if self._match(NIL) return LiteralExpr(nil)

    if self._match(BIN_NUMBER, HEX_NUMBER, OCT_NUMBER, REG_NUMBER, LITERAL)
      return LiteralExpr(self._previous().literal)

    if self._match(IDENTIFIER) return IdentifierExpr(self._previous().literal)

    if self._match(LPAREN) return self._grouping()
  }

  /**
   * factor expressions
   */
  _unary() {
    if self._match(BANG, MINUS, TILDE) {
      var op = self._previous()
      var right = self._primary()
      return UnaryExpr(op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = BinaryExpr(expr, op, right)
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
      expr = ConditionExpr(expr, truth, self._or())
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

  ### STATEMENTS START

  _echo() {
    var val = self._expression()
    self._end_statement()
    return EchoStmt(val)
  }

  _expr_stmt() {
    var val = self._expression()
    self._end_statement()
    return ExprStmt(val)
  }

  _block() {
    self._block_count++

    var val = []
    self._ignore_newline()

    while !self._check(RBRACE) and !self._is_at_end() {
      val.append(self._declaration())
    }

    self._consume(RBRACE, 'expected } after block')
    self._block_count--

    return val
  }

  _if() {
    var expr = self._expression()
    var body = self._statement()
    
    if self._match(ELSE) {
      return IfStmt(expr, body, self._statement())
    }

    return IfStmt(expr, body, nil)
  }

  /**
   * parse Blade statements
   */
  _statement() {
    self._ignore_newline()

    var result

    if self._match(ECHO) {
      result = self._echo()
    } else if self._match(IF) {
      result = self._if()
    } else if self._match(WHILE) {
      
    } else if self._match(ITER) {
      
    } else if self._match(FOR) {
      
    } else if self._match(USING) {
      
    } else if self._match(CONTINUE) {
      
    } else if self._match(BREAK) {
      
    } else if self._match(RETURN) {
      
    } else if self._match(ASSERT) {
      
    } else if self._match(DIE) {
      
    } else if self._match(LBRACE) {
      result = self._block()
    } else if self._match(IMPORT) {
      
    } else if self._match(TRUE) {
      
    } else {
      result = self._expr_stmt()
    }

    self._ignore_newline()

    return result
  }

  _declaration() {
    if self._match(CLASS) {

    } else if self._match(VAR) {

    }

    # TODO: Remove this when method is implemented correctly
    self._advance()
  }

  ### STATEMENTS END

  parse() {
    var statements = []

    while !self._is_at_end()
      statements.append(self._statement())

    return statements
  }
}