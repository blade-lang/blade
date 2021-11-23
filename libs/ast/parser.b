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

  _expr_stmt(is_iter) {
    var val = self._expression()
    if !is_iter self._end_statement()
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

  _while() {
    return WhileStmt(self._expression(), self._statement())
  }

  _for() {
    var vars = []
    while self._match(IDENTIFIER) {
      vars.append(self._previous())
    }

    self._consume(IN, "expected 'in' after for statement variables")

    return ForStmt(vars, self._expression(), self._statement())
  }

  _assert() {
    var message
    var expr = self._expression()

    if self._match(COMMA) message = self._expression()
    return AssertStmt(expr, message)
  }

  _using() {
    var expr = self._expression()
    var cases = {}
    var default_case

    self._consume(LBRACE, 'expected { after using expression')
    self._ignore_newline()

    var state = 0

    while !self._match(RBRACE) and !self._check(EOF) {
      if self._match(WHEN) or self._match(DEFAULT) {
        if state == 1 
          die ParseException('cannot have another case after a default case')

        if self._previous().type == WHEN {
          cases[self._expression()] = self._statement()
        } else {
          state = 1
          default_case = self._statement()
        }
      } else {
        die ParseException('Invalid switch statement')
      }
    }

    return UsingStmt(expr, cases, default_case)
  }

  _import() {
    var path = []

    while !self._match(NEWLINE, EOF) {
      self._advance()
      path.append(self._previous().literal)
    }

    return ImportStmt(''.join(path))
  }

  _try() {
    self._consume(LBRACE, 'expected { after try')
    var body = self._block()
    var exception_type, exception_var, catch_body, finally_body
    var has_catch = false, has_finally = false

    if self._match(CATCH) {
      self._consume(IDENTIFIER, 'expected exception name')
      exception_type = self._previous().literal

      if self._match(AS) {
        self._consume(IDENTIFIER, 'expected exception variable')
        exception_var = self._previous().literal
      }

      self._consume(LBRACE, 'expected { after catch expression')
      catch_body = self._block()
      has_catch = true
    }
    
    if self._match(FINALLY) {
      has_finally = true
      self._consume(LBRACE, 'expected { after finally')
      finally_body = self._block()
    }

    if !has_catch and !has_finally
      die ParseException('invalid try statement')

    var catch_stmt, finally_stmt
    if has_catch catch_stmt = CatchStmt(exception_type, exception_var, catch_body)
    if has_finally finally_stmt = FinallyStmt(finally_body)

    return TryStmt(body, catch_stmt, finally_stmt)
  }

  _iter() {
    var decl
    if !self._check(SEMICOLON) {
      self._consume(VAR, 'expected variable declaration')
      decl = self._var()
    }
    self._consume(SEMICOLON, 'expected ;')

    var condition
    if !self._check(SEMICOLON) condition = self._expression()
    self._consume(SEMICOLON, 'expected ;')

    var iterator
    if !self._check(LBRACE) iterator = self._expr_stmt(true)
    self._consume(LBRACE, 'expected {')

    var body = self._block()
    return IterStmt(decl, condition, iterator, body)
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
      result = self._while()
    } else if self._match(ITER) {
      result = self._iter()
    } else if self._match(FOR) {
      result = self._for()
    } else if self._match(USING) {
      result = self._using()
    } else if self._match(CONTINUE) {
      result = ContinueStmt()
    } else if self._match(BREAK) {
      result = BreakStmt()
    } else if self._match(RETURN) {
      result = ReturnStmt(self._expression())
    } else if self._match(ASSERT) {
      result = self._assert()
    } else if self._match(DIE) {
      result = DieStmt(self._expression())
    } else if self._match(LBRACE) {
      result = self._block()
    } else if self._match(IMPORT) {
      result = self._import()
    } else if self._match(TRY) {
      result = self._try()
    } else {
      result = self._expr_stmt(false)
    }

    self._ignore_newline()

    return result
  }

  ### STATEMENTS END

  ### DECLARATIONS START

  _var() {
    self._consume(IDENTIFIER, 'expected variable name')
    var result = self._previous().literal

    if self._match(EQUAL)
      result = VarDecl(result, self._expression())
    else result = VarDecl(result, nil)

    if self._check(COMMA) {
      result = [result] # we want to return an array of declarations

      while self._match(COMMA) {
        self._consume(IDENTIFIER, 'expected variable name')
        var r = self._previous().literal

        if self._match(EQUAL)
          r = VarDecl(r, self._expression())
        else r = VarDecl(r, nil)

        result.append(r)
      }
    }

    return result
  }

  _def() {
    self._consume(IDENTIFIER, 'expected function name')
    var name = self._previous().literal
    var params = []

    self._consume(LPAREN, 'expected ( after function name')
    while self._match(IDENTIFIER) {
      params.append(self._previous().literal)

      if !self._check(RPAREN)
        self._consume(COMMA, 'expected , between function params')
    }
    self._consume(RPAREN, 'expected ) after function args')
    self._consume(LBRACE, 'expected { after function declaration')
    var body = self._block()

    return FunctionDecl(name, params, body)
  }

  _declaration() {
    self._ignore_newline()
    
    var result

    if self._match(VAR) {
      result = self._var()
    } else if self._match(DEF) {
      result = self._def()
    } else if self._match(CLASS) {

    } else {
      result = self._statement()
    }

    self._ignore_newline()
    return result
  }

  ### DECLARATIONS END

  parse() {
    var declarations = []

    while !self._is_at_end()
    declarations.append(self._declaration())

    return declarations
  }
}