#!-- part of the ast module

import .token { * }
import .expr { * }
import .stmt { * }
import .decl { * }
import .defn { * }
import .interface { * }
import .exception { * }

var _assigners_ = [
  TokenType.EQUAL, TokenType.PLUS_EQ, TokenType.MINUS_EQ, 
  TokenType.PERCENT_EQ, TokenType.DIVIDE_EQ, TokenType.MULTIPLY_EQ, 
  TokenType.FLOOR_EQ, TokenType.POW_EQ, TokenType.AMP_EQ, 
  TokenType.BAR_EQ, TokenType.TILDE_EQ, TokenType.XOR_EQ,
  TokenType.LSHIFT_EQ, TokenType.RSHIFT_EQ, TokenType.URSHIFT_EQ,
]

# NOTE: ++, and -- are not primary operators in Blade.
var _ops = [
  TokenType.PLUS, #  +
  TokenType.MINUS, #  -
  TokenType.MULTIPLY, #  *
  TokenType.POW, #  **
  TokenType.DIVIDE, #  '/'
  TokenType.FLOOR, #  '//'
  TokenType.EQUAL, #  =
  TokenType.LESS, #  <
  TokenType.LSHIFT, #  <<
  TokenType.GREATER, #  >
  TokenType.RSHIFT, #  >>
  TokenType.URSHIFT, #  >>>
  TokenType.PERCENT, #  %
  TokenType.AMP, #  &
  TokenType.BAR, #  |
  TokenType.TILDE, #  ~
  TokenType.XOR, #  ^
]

# Helper function to get documentation string
def _get_doc_string(data) {
  return '${data}\n'.replace('/\\s*\\*(.*)\n/', '$1\n ').trim()
}


/**
 * Parses raw Blade tokens and produces an Abstract Syntax Tree.
 * 
 * @printable
 */
class Parser {

  /**
   * @param list[Token] tokens
   * @param string? path
   * @constructor 
   */
  Parser(tokens, path) {
    if !is_list(tokens)
      raise TypeError('list expected in argument 1 (tokens)')
    if !is_string(path)
      raise TypeError('string expected in argument 2 (path)')

    # set instance variable token
    self._tokens = tokens

    # the current path of the parser
    self._path = path
    # the nested scope depth
    self._block_count = 0
    # a pointer to the next token waiting to be parsed
    self._current = 0
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
   * checks to see if the current token has any of the given types
   * TokenType._A list alternative to the default._
   */
  _match_in(args) {
    for t in args {
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
    if self._is_at_end() and type != TokenType.EOF return false
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
    return self._peek().type == TokenType.EOF
  }

  /**
   * returns the current token we have yet to consume
   */
  _peek() {
    return self._tokens[self._current]
  }

  /**
   * returns the most recently consumed token
   */
  _previous() {
    return self._tokens[self._current - 1]
  }

  /**
   * continues only if the next token is of the given type.
   * Otherwise, it reports the given message as a ParseException
   */
  _consume(type, message) {
    if self._check(type) return self._advance()
    raise ParseException(message, self._peek())
  }

  /**
   * continues only if the next token is one of the given type.
   * Otherwise, it reports the given message as a ParseException
   */
  _consume_any(message, ...) {
    for t in __args__ {
      if self._check(t) return self._advance()
    }
    raise ParseException(message, self._peek())
  }

  _consume_one_of(message, opts) {
    for t in opts {
      if self._check(t) return self._advance()
    }
    raise ParseException(message, self._peek())
  }

  _get_doc_defn_data() {
    return self._previous().literal[3,-2].replace('/^\s*\*\s?/m', '').trim()
  }

  _end_statement() {
    if self._match(TokenType.EOF) or self._is_at_end() return

    if self._block_count > 0 and self._check(TokenType.RBRACE) return

    if self._match(TokenType.SEMICOLON, TokenType.COMMENT) {
      while self._match(TokenType.NEWLINE, TokenType.SEMICOLON, TokenType.COMMENT) {}
      return
    }

    self._consume(TokenType.NEWLINE, 'end of statement expected')

    while self._match(TokenType.NEWLINE, TokenType.SEMICOLON) {}
  }

  /**
   * ignores consecutive newlines
   */
  _ignore_newline() {
    while self._match(TokenType.NEWLINE, TokenType.COMMENT) {}
  }

  ### EXPRESSIONS START

  /**
   * grouped expressions
   */
  _grouping() {
    self._ignore_newline()
    var expr = self._expression()
    self._ignore_newline()
    self._consume(TokenType.RPAREN, "')' Expected after expression")
    return GroupExpr(expr)
  }

  /**
   * completes a method call expression
   */
  _finish_call(callee) {
    self._ignore_newline()
    var args = []

    if !self._check(TokenType.RPAREN) {
      args.append(self._expression())

      while self._match(TokenType.COMMA) {
        self._ignore_newline()
        args.append(self._expression())
      }
    }

    self._ignore_newline()
    self._consume(TokenType.RPAREN, "')' expected after args")
    return CallExpr(callee, args)
  }

  /**
   * completes index access expressions
   */
  _finish_index(callee) {
      self._ignore_newline()
    var args = [self._expression()]

    if self._match(TokenType.COMMA) {
      self._ignore_newline()
      args.append(self._expression())
    }

    self._ignore_newline()
    self._consume(TokenType.RBRACKET, "']' expected at end of indexer")
    return IndexExpr(callee, args)
  }

  /**
   * completes a getter or setter call
   */
  _finish_dot(expr) {
    self._ignore_newline()
    var prop = self._consume(TokenType.IDENTIFIER, 'property name expected').literal

    if self._match_in(_assigners_) {
      expr = SetExpr(expr, prop, self._expression())
    } else {
      expr = GetExpr(expr, prop)
    }

    return expr
  }

  /**
   * string interpolations
   */
  _interpolation() {
    var data = []
    while self._match(TokenType.INTERPOLATION, TokenType.LITERAL) {
      if self._previous().literal.length() > 0
        data.append(LiteralExpr(self._previous().literal))

      data.append(self._expression())
    }

    if self._previous().literal.length() > 0
      data.append(LiteralExpr(self._previous().literal))

    return InterpolationExpr(data)
  }

  /**
   * primary expressions
   */
  _primary() {
    if self._match(TokenType.FALSE) return LiteralExpr(false)
    if self._match(TokenType.TRUE) return LiteralExpr(true)
    if self._match(TokenType.NIL) return LiteralExpr(nil)
    if self._match(TokenType.SELF) return LiteralExpr('::self::')
    if self._match(TokenType.PARENT) return LiteralExpr('::parent::')

    if self._check(TokenType.INTERPOLATION) return self._interpolation()

    if self._match(TokenType.BIN_NUMBER, TokenType.HEX_NUMBER, TokenType.OCT_NUMBER, TokenType.REG_NUMBER, TokenType.LITERAL)
      return LiteralExpr(self._previous().literal)

    if self._match(TokenType.IDENTIFIER) return IdentifierExpr(self._previous().literal)

    if self._match(TokenType.LPAREN) return self._grouping()
    if self._match(TokenType.LBRACE) return self._dict()
    if self._match(TokenType.LBRACKET) return self._list()
    if self._match(TokenType.AT) return self._anonymous()

    return nil
  }

  /**
   * method, property and index calls
   */
  _call() {
    var expr = self._primary()

    while true {
      if self._match(TokenType.DOT) {
        expr = self._finish_dot(expr)
      } else if self._match(TokenType.LPAREN) {
        expr = self._finish_call(expr)
      } else if self._match(TokenType.LBRACKET) {
        expr = self._finish_index(expr)
      } else {
        break
      }
    }

    return expr
  }

  /**
   * expressions that assign value (++, --)
   */
  _assign_expr() {
    var expr = self._call()

    if self._match(TokenType.INCREMENT) {
      expr = AssignStmt(expr, '++', nil)
    } else if self._match(TokenType.DECREMENT) {
      expr = AssignStmt(expr, '--', nil)
    }

    return expr
  }

  /**
   * factor expressions
   */
  _unary() {
    if self._match(TokenType.BANG, TokenType.MINUS, TokenType.TILDE) {
      var op = self._previous().literal
      self._ignore_newline()
      var right = self._assign_expr()
      return UnaryExpr(op, right)
    }

    return self._assign_expr()
  }

  /**
   * factor expressions
   */
  _factor() {
    while self._match(TokenType.DOC){}
    var expr = self._unary()
    while self._match(TokenType.DOC){}

    while self._match(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.PERCENT, TokenType.POW, TokenType.FLOOR) {
      var op = self._previous().literal
      self._ignore_newline()

      while self._match(TokenType.DOC){}
      var right = self._unary()
      while self._match(TokenType.DOC){}

      expr = BinaryExpr(expr, op, right)
    }

    return expr
  }

  /**
   * term expressions
   */
  _term() {
    var expr = self._factor()

    while self._match(TokenType.PLUS, TokenType.MINUS) {
      var op = self._previous().literal
      self._ignore_newline()
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

    while self._match(TokenType.RANGE) {
      self._ignore_newline()
      var op = '..'
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

    while self._match(TokenType.LSHIFT, TokenType.RSHIFT, TokenType.URSHIFT) {
      var op = self._previous().literal
      self._ignore_newline()
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

    while self._match(TokenType.AMP) {
      self._ignore_newline()
      var op = '&'
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

    while self._match(TokenType.XOR) {
      self._ignore_newline()
      var op = '^'
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

    while self._match(TokenType.BAR) {
      self._ignore_newline()
      var op = '|'
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

    while self._match(TokenType.GREATER, TokenType.GREATER_EQ, TokenType.LESS, TokenType.LESS_EQ) {
      var op = self._previous().literal
      self._ignore_newline()
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

    while self._match(TokenType.BANG_EQ, TokenType.EQUAL_EQ) {
      var op = self._previous().literal
      self._ignore_newline()
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

    while self._match(TokenType.AND) {
      self._ignore_newline()
      var op = 'and'
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

    while self._match(TokenType.OR) {
      self._ignore_newline()
      var op = 'or'
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

    if self._match(TokenType.QUESTION) {
      self._ignore_newline()
      var truth = self._conditional()
      self._consume(TokenType.COLON, "':' expected in ternary operation")
      self._ignore_newline()
      expr = ConditionExpr(expr, truth, self._conditional())
    }

    return expr
  }

  /**
   * assignment to existing vars
   */
  _assignment() {
    var expr = self._conditional()

    if self._match_in(_assigners_) {
      var type = self._previous().literal
      self._ignore_newline()

      expr = AssignStmt(expr, type, self._assignment())
    }

    return expr
  }

  /**
   * parses an expression
   */
  _expression() {
    return self._assignment()
  }

  /**
   * parses a dictionary
   */
  _dict() {
    self._ignore_newline()
    var keys = [], values = []

    if !self._check(TokenType.RBRACE) {
      do {
        while self._match(TokenType.DOC) {}
        self._ignore_newline()
        while self._match(TokenType.DOC) {}

        if !self._check(TokenType.RBRACE) {
          var auto_value
          if self._match(TokenType.IDENTIFIER) {
            keys.append(self._previous().literal)
          } else {
            keys.append(self._expression())
          }
          self._ignore_newline()

          if !self._match(TokenType.COLON) {
            auto_value = self._previous().literal
          } else {
            self._ignore_newline()
            values.append(self._expression())
          }

          self._ignore_newline()
        }
      } while(self._match(TokenType.COMMA))
    }

    self._ignore_newline()
    self._consume(TokenType.RBRACE, "'}' expected after dictionary")
    return DictExpr(keys, values)
  }

  /**
   * parses a list
   */
  _list() {
    self._ignore_newline()
    var items = []

    if !self._check(TokenType.RBRACKET) {
      do {
        while self._match(TokenType.DOC) {}
        self._ignore_newline()
        while self._match(TokenType.DOC) {}
  
        if !self._check(TokenType.RBRACKET) {
          items.append(self._expression())
          self._ignore_newline()
        }
      } while(self._match(TokenType.COMMA))
    }

    self._ignore_newline()
    self._consume(TokenType.RBRACKET, "expected ']' at the end of list")
    return ListExpr(items)
  }

  ### EXPRESSIONS END

  ### STATEMENTS START

  /**
   * echo statement
   */
  _echo() {
    var val = self._expression()
    self._end_statement()
    return EchoStmt(val)
  }

  /**
   * expression statement
   */
  _expr_stmt(is_iter) {
    var val = self._expression()
    if !is_iter self._end_statement()
    return ExprStmt(val)
  }

  /**
   * block scopes
   */
  _block() {
    self._block_count++

    var val = []
    self._ignore_newline()

    while !self._check(TokenType.RBRACE) and !self._is_at_end()
      val.append(self._declaration())

    self._consume(TokenType.RBRACE, "'}' expected after block")
    self._block_count--

    return BlockStmt(val)
  }

  /**
   * if statements
   */
  _if() {
    var expr = self._expression()
    var body = self._statement()
    
    if self._match(TokenType.ELSE) return IfStmt(expr, body, self._statement())

    return IfStmt(expr, body, nil)
  }

  /**
   * while loops
   */
  _while() {
    return WhileStmt(self._expression(), self._statement())
  }

  /**
   * while loops
   */
  _do_while() {
    var body = self._statement()
    self._consume(TokenType.WHILE, "'while' expected after do body")
    return DoWhileStmt(body, self._expression())
  }

  /**
   * for loops
   */
  _for() {
    var vars = [self._consume(TokenType.IDENTIFIER, 'variable name expected')]

    if self._match(TokenType.COMMA)
      vars.append(self._consume(TokenType.IDENTIFIER, 'variable name expected'))

    self._consume(TokenType.IN, "'in' expected after for statement variables")

    return ForStmt(vars, self._expression(), self._statement())
  }

  /**
   * assert statements
   */
  _assert() {
    var message
    var expr = self._expression()

    if self._match(TokenType.COMMA) message = self._expression()
    return AssertStmt(expr, message)
  }

  /**
   * using statements
   */
  _using() {
    var expr = self._expression()
    var cases = []
    var default_case

    self._consume(TokenType.LBRACE, "'{' expected after using expression")
    self._ignore_newline()

    var state = 0

    while !self._match(TokenType.RBRACE) and !self._check(TokenType.EOF) {
      if self._match(TokenType.WHEN, TokenType.DEFAULT, TokenType.COMMENT, TokenType.DOC, TokenType.NEWLINE) {
        if state == 1 
          raise ParseException("'when' cannot exist after a default", self._previous())

        if [TokenType.DOC, TokenType.COMMENT, TokenType.NEWLINE].contains(self._previous().type) {}
        else if self._previous().type == TokenType.WHEN {
          var conditions = []

          do {
            self._ignore_newline()
            conditions.append(self._expression())
          } while self._match(TokenType.COMMA)

          var stmt = self._statement()
          cases.append(CaseStmt(conditions, stmt))

        } else {
          state = 1
          default_case = self._statement()
        }
      } else {
        raise ParseException('Invalid using statement', self._previous())
      }
    }

    return UsingStmt(expr, cases, default_case)
  }

  /**
   * import statements
   */
  _import() {
    var path = []
    var elements = []

    while !self._match(TokenType.NEWLINE, TokenType.EOF, TokenType.LBRACE) {
      self._advance()
      path.append(self._previous().literal)
    }

    if self._previous().type == TokenType.LBRACE {
      var scan = true
      while !self._check(TokenType.RBRACE) and scan {
        self._ignore_newline()
        elements.append(self._consume_any('identifier expected', TokenType.IDENTIFIER, TokenType.MULTIPLY).literal)
        if !self._match(TokenType.COMMA)
          scan = false
        self._ignore_newline()
      }

      self._consume(TokenType.RBRACE, "'}' expected at end of selective import")
    }

    return ImportStmt(''.join(path), elements)
  }

  /**
   * catch... blocks
   */
  _catch() {
    self._consume(TokenType.LBRACE, "'{' expected after catch")
    var body = self._block()

    var exception_var
    if self._match(TokenType.AS) {
      if self._check(TokenType.IDENTIFIER) {
        self._consume(TokenType.IDENTIFIER, 'exception variable expected')
        exception_var = self._previous().literal
      }
    }

    return CatchStmt(body, exception_var)
  }

  /**
   * iter loops
   */
  _iter() {
    var decl
    if !self._check(TokenType.SEMICOLON) {
      if self._check(TokenType.VAR) {
        self._consume(TokenType.VAR, 'variable declaration expected')
      }
      decl = self._var()
    }
    self._consume(TokenType.SEMICOLON, "';' expected")
    self._ignore_newline()

    var condition
    if !self._check(TokenType.SEMICOLON) condition = self._expression()
    self._consume(TokenType.SEMICOLON, "';' expected")
    self._ignore_newline()

    var iterator
    if !self._check(TokenType.LBRACE) {
      do {
        iterator = self._expr_stmt(true)
        self._ignore_newline()
      } while self._match(TokenType.COMMA)
    }

    var body = self._statement()
    return IterStmt(decl, condition, iterator, body)
  }

  /**
   * parse Blade statements
   */
  _statement() {
    self._ignore_newline()

    var result

    if self._match(TokenType.ECHO) {
      result = self._echo()
    } else if self._match(TokenType.IF) {
      result = self._if()
    } else if self._match(TokenType.WHILE) {
      result = self._while()
    } else if self._match(TokenType.DO) {
      result = self._do_while()
    } else if self._match(TokenType.ITER) {
      result = self._iter()
    } else if self._match(TokenType.FOR) {
      result = self._for()
    } else if self._match(TokenType.USING) {
      result = self._using()
    } else if self._match(TokenType.CONTINUE) {
      result = ContinueStmt()
    } else if self._match(TokenType.BREAK) {
      result = BreakStmt()
    } else if self._match(TokenType.RETURN) {
      result = ReturnStmt(self._expression())
    } else if self._match(TokenType.ASSERT) {
      result = self._assert()
    } else if self._match(TokenType.RAISE) {
      result = RaiseStmt(self._expression())
    } else if self._match(TokenType.LBRACE) {
      result = self._block()
    } else if self._match(TokenType.IMPORT) {
      result = self._import()
    } else if self._match(TokenType.CATCH) {
      result = self._catch()
    } else if self._match(TokenType.COMMENT) {
      result = CommentStmt(self._previous().literal[1,].trim())
    } else if self._match(TokenType.DOC) {
      result = DocDefn(self._get_doc_defn_data())
    } else {
      result = self._expr_stmt(false)
    }

    self._ignore_newline()

    return result
  }

  ### STATEMENTS END

  ### DECLARATIONS START

  /**
   * variable declarations
   */
  _var() {
    self._consume(TokenType.IDENTIFIER, 'variable name expected')
    var result = self._previous().literal

    if self._match(TokenType.EQUAL)
      result = VarDecl(result, self._expression())
    else result = VarDecl(result, nil)

    if self._check(TokenType.COMMA) {
      result = [result] # we want to return an array of declarations

      while self._match(TokenType.COMMA) {
        self._ignore_newline()
        self._consume(TokenType.IDENTIFIER, 'variable name expected')
        var r = self._previous().literal

        if self._match(TokenType.EQUAL)
          r = VarDecl(r, self._expression())
        else r = VarDecl(r, nil)

        result.append(r)
      }
    }

    return result
  }

  /**
   * anonymous functions
   */
  _anonymous() {
    var params = []

    if self._check(TokenType.LPAREN) {
      self._consume(TokenType.LPAREN, "expected '(' at start of anonymous function")
  
      while !self._check(TokenType.RPAREN) {
        params.append(self._consume_any('parameter name expected', TokenType.IDENTIFIER, TokenType.TRI_DOT).literal)
  
        if !self._check(TokenType.RPAREN)
          self._consume(TokenType.COMMA, "',' expected between function params")
      }
  
      self._consume(TokenType.RPAREN, "expected ')' after anonymous function parameters")
    }

    self._consume(TokenType.LBRACE, "'{' expected after function declaration")
    var body = self._block()

    return FunctionDecl('', params, body)
  }

  /**
   * function definitions
   */
  _def() {
    self._consume(TokenType.IDENTIFIER, 'function name expected')
    var name = self._previous().literal
    var params = []

    self._consume(TokenType.LPAREN, "'(' expected after function name")
    while self._match(TokenType.IDENTIFIER, TokenType.TRI_DOT) {
      params.append(self._previous().literal)

      if !self._check(TokenType.RPAREN) {
        self._consume(TokenType.COMMA, "',' expected between function arguments")
        self._ignore_newline()
      }
    }
    self._consume(TokenType.RPAREN, "')' expected after function arguments")
    self._consume(TokenType.LBRACE, "'{' expected after function declaration")
    var body = self._block()

    return FunctionDecl(name, params, body)
  }

  /**
   * class fields
   */
  _class_field(is_static) {
    self._consume(TokenType.IDENTIFIER, 'class property name expected')
    var name = self._previous().literal, value

    if self._match(TokenType.EQUAL) value = self._expression()
    self._end_statement()
    self._ignore_newline()

    return PropertyDecl(name, value, is_static)
  }

  /**
   * class operator
   */
  _class_operator() {
    self._consume_one_of('non-assignment operator expected', _ops)
    var name = self._previous().literal

    self._consume(TokenType.LBRACE, "'{' expected after operator declaration")
    var body = self._block()

    return MethodDecl(name, [], body, false)
  }

  /**
   * class methods
   */
  _method(is_static) {
    self._consume_any('method name expected', TokenType.IDENTIFIER, TokenType.DECORATOR)
    var name = self._previous().literal
    var params = []

    self._consume(TokenType.LPAREN, "'(' expected after method name")
    while self._match(TokenType.IDENTIFIER, TokenType.TRI_DOT) {
      params.append(self._previous().literal)

      if !self._check(TokenType.RPAREN) {
        self._consume(TokenType.COMMA, "',' expected between method arguments")
        self._ignore_newline()
      }
    }
    self._consume(TokenType.RPAREN, "')' expected after method arguments")
    self._consume(TokenType.LBRACE, "'{' expected after method declaration")
    var body = self._block()

    return MethodDecl(name, params, body, is_static)
  }

  /**
   * classes
   */
  _class() {
    var properties = [], methods = [], operators = []

    self._consume(TokenType.IDENTIFIER, 'class name expected')
    var name = self._previous().literal, superclass

    if self._match(TokenType.LESS) {
      self._consume(TokenType.IDENTIFIER, 'super class name expected')
      superclass = self._previous().literal
    }

    self._ignore_newline()
    self._consume(TokenType.LBRACE, "'{' expected after class declaration")
    self._ignore_newline()

    while !self._check(TokenType.RBRACE) and !self._check(TokenType.EOF) {
      var is_static = false
      var doc

      self._ignore_newline()
      while self._match(TokenType.COMMENT) {}
      self._ignore_newline()

      if self._match(TokenType.DOC)
        doc = DocDefn(self._get_doc_defn_data())

      self._ignore_newline()

      if self._match(TokenType.STATIC) is_static = true

      if self._match(TokenType.VAR) {
        var prop = self._class_field(is_static)
        if doc and !prop.name.starts_with('_') 
          prop.doc = _get_doc_string(doc.data)
        properties.append(prop)
      } else if self._match(TokenType.DEF) {
        operators.append(self._class_operator())
        self._ignore_newline()
      } else {
        var method = self._method(is_static)
        if doc and !method.name.starts_with('_') and !method.name.starts_with('@') 
          method.doc = _get_doc_string(doc.data)
        methods.append(method)
        self._ignore_newline()
      }
    }

    self._consume(TokenType.RBRACE, "'{' expected at end of class definition")

    return ClassDecl(name, superclass, properties, methods, operators)
  }

  /**
   * Blade's declarations
   */
  _declaration() {
    self._ignore_newline()
    
    var result

    if self._match(TokenType.VAR) {
      result = self._var()
    } else if self._match(TokenType.DEF) {
      result = self._def()
    } else if self._match(TokenType.CLASS) {
      result = self._class()
    } else if self._match(TokenType.COMMENT) {
      result = CommentStmt(self._previous().literal[1,].trim())
    } else if self._match(TokenType.DOC) {
      result = DocDefn(self._get_doc_defn_data())
    } else if self._match(TokenType.LBRACE) {
      if !self._check(TokenType.NEWLINE) and self._block_count == 0 
        result = self._dict()
      else result = self._block()
    }  else {
      result = self._statement()
    }

    self._ignore_newline()
    return result
  }

  ### DECLARATIONS END

  /**
   * Parses the raw source tokens passed into relevant class and
   * outputs a stream of AST objects that can be one of
   * Expr (expressions), Stmt (statements) or Decl (declarations).
   * 
   * @returns ParseResult
   */
  parse() {
    var result = ParseResult(self._path)

    while !self._is_at_end() {
      var declaration = self._declaration()
      declaration.file = self._path
      result.append(declaration)
    }

    return result
  }

  @to_string() {
    return "<ast::Parser path='${self._path}' tokens=${self._tokens.length()}>"
  }
}