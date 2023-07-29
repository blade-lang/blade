#!-- part of the ast module

import .token { * }
import .expr { * }
import .stmt { * }
import .decl { * }
import .defn { * }
import .interface { * }
import .exception { * }

var _assigners_ = [EQUAL, PLUS_EQ, MINUS_EQ, PERCENT_EQ, DIVIDE_EQ,
  MULTIPLY_EQ, FLOOR_EQ, POW_EQ, AMP_EQ, BAR_EQ, TILDE_EQ, XOR_EQ,
  LSHIFT_EQ, RSHIFT_EQ]

# Helper function to get documentation string
def _get_doc_string(data) {
  return '${data}\n'.replace('/\\s*\\*(.*)\n/', '$1\n ').trim()
}


/**
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
   * Parser(tokens: []Token)
   * @constructor 
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
   * checks to see if the current token has any of the given types
   * _A list alternative to the default._
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

  /**
   * continues only if the next token is one of the given type.
   * Otherwise, it reports the given message as a ParseException
   */
  _consume_any(message, ...) {
    for t in __args__ {
      if self._check(t) return self._advance()
    }
    die ParseException(self._peek(), message)
  }

  _end_statement() {
    if self._match(EOF) or self._is_at_end() return

    if self._block_count > 0 and self._check(RBRACE) return

    if self._match(SEMICOLON, COMMENT) {
      while self._match(NEWLINE, SEMICOLON, COMMENT) {}
      return
    }

    self._consume(NEWLINE, 'end of statement expected')

    while self._match(NEWLINE, SEMICOLON) {}
  }

  /**
   * ignores consecutive newlines
   */
  _ignore_newline() {
    while self._match(NEWLINE, COMMENT) {}
  }

  ### EXPRESSIONS START

  /**
   * grouped expressions
   */
  _grouping() {
    self._ignore_newline()
    var expr = self._expression()
    self._ignore_newline()
    self._consume(RPAREN, "')' Expected after expression")
    return GroupExpr(expr)
  }

  /**
   * completes a method call expression
   */
  _finish_call(callee) {
    self._ignore_newline()
    var args = []

    if !self._check(RPAREN) {
      args.append(self._expression())

      while self._match(COMMA) {
        self._ignore_newline()
        args.append(self._expression())
      }
    }

    self._ignore_newline()
    self._consume(RPAREN, "')' expected after args")
    return CallExpr(callee, args)
  }

  /**
   * completes index access expressions
   */
  _finish_index(callee) {
    var args = [self._expression()]

    if self._match(COMMA) {
      self._ignore_newline()
      args.append(self._expression())
    }

    self._consume(RBRACKET, "']' expected at end of indexer")
    return IndexExpr(args)
  }

  /**
   * completes a getter or setter call
   */
  _finish_dot(expr) {
    self._ignore_newline()
    var prop = self._consume(IDENTIFIER, 'property name expected').literal

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
    while self._match(INTERPOLATION, LITERAL) {
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
    if self._match(FALSE) return LiteralExpr(false)
    if self._match(TRUE) return LiteralExpr(true)
    if self._match(NIL) return LiteralExpr(nil)
    if self._match(SELF) return LiteralExpr('::self::')
    if self._match(PARENT) return LiteralExpr('::parent::')

    if self._match(BIN_NUMBER, HEX_NUMBER, OCT_NUMBER, REG_NUMBER, LITERAL)
      return LiteralExpr(self._previous().literal)

    if self._check(INTERPOLATION) return self._interpolation()

    if self._match(IDENTIFIER) return IdentifierExpr(self._previous().literal)

    if self._match(LPAREN) return self._grouping()
    if self._match(LBRACE) return self._dict()
    if self._match(LBRACKET) return self._list()
    if self._match(BAR) return self._anonymous_compat()
    if self._match(AT) return self._anonymous()

    return nil
  }

  /**
   * method, property and index calls
   */
  _call() {
    var expr = self._primary()

    while true {
      if self._match(DOT) {
        expr = self._finish_dot(expr)
      } else if self._match(LPAREN) {
        expr = self._finish_call(expr)
      } else if self._match(LBRACKET) {
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

    if self._match(INCREMENT) {
      expr = AssignStmt(expr, '++', nil)
    } else if self._match(DECREMENT) {
      expr = AssignStmt(expr, '--', nil)
    }

    return expr
  }

  /**
   * factor expressions
   */
  _unary() {
    if self._match(BANG, MINUS, TILDE) {
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
    var expr = self._unary()

    while self._match(MULTIPLY, DIVIDE, PERCENT, POW, FLOOR) {
      var op = self._previous().literal
      self._ignore_newline()
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

    while self._match(RANGE) {
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

    while self._match(LSHIFT, RSHIFT) {
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

    while self._match(AMP) {
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

    while self._match(XOR) {
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

    while self._match(BAR) {
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

    while self._match(GREATER, GREATER_EQ, LESS, LESS_EQ) {
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

    while self._match(BANG_EQ, EQUAL_EQ) {
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

    while self._match(AND) {
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

    while self._match(OR) {
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

    if self._match(QUESTION) {
      self._ignore_newline()
      var truth = self._or()
      self._consume(COLON, "':' expected in tenary operation")
      self._ignore_newline()
      expr = ConditionExpr(expr, truth, self._or())
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

    if !self._check(RBRACE) {
      do {
        self._ignore_newline()

        if !self._check(RBRACE) {
          var auto_value
          if self._match(IDENTIFIER) {
            keys.append(self._previous().literal)
          } else {
            keys.append(self._expression())
          }
          self._ignore_newline()

          if !self._match(COLON) {
            auto_value = self._previous().literal
          } else {
            self._ignore_newline()
            values.append(self._expression())
          }

          self._ignore_newline()
        }
      } while(self._match(COMMA))
    }

    self._ignore_newline()
    self._consume(RBRACE, "'}' expected after dictionary")
    return DictExpr(keys, values)
  }

  /**
   * parses a list
   */
  _list() {
    self._ignore_newline()
    var items = []

    if !self._check(RBRACKET) {
      do {
        self._ignore_newline()
  
        if !self._check(RBRACKET) {
          items.append(self._expression())
          self._ignore_newline()
        }
      } while(self._match(COMMA))
    }

    self._ignore_newline()
    self._consume(RBRACKET, "expected ']' at the end of list")
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

    while !self._check(RBRACE) and !self._is_at_end()
      val.append(self._declaration())

    self._consume(RBRACE, "'}' expected after block")
    self._block_count--

    return BlockStmt(val)
  }

  /**
   * if statements
   */
  _if() {
    var expr = self._expression()
    var body = self._statement()
    
    if self._match(ELSE) return IfStmt(expr, body, self._statement())

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
    self._consume(WHILE, "'while' expected after do body")
    return DoWhileStmt(body, self._expression())
  }

  /**
   * for loops
   */
  _for() {
    var vars = [self._consume(IDENTIFIER, 'variable name expected')]

    if self._match(COMMA)
      vars.append(self._consume(IDENTIFIER, 'variable name expected'))

    self._consume(IN, "'in' expected after for statement variables")

    return ForStmt(vars, self._expression(), self._statement())
  }

  /**
   * assert statements
   */
  _assert() {
    var message
    var expr = self._expression()

    if self._match(COMMA) message = self._expression()
    return AssertStmt(expr, message)
  }

  /**
   * using statements
   */
  _using() {
    var expr = self._expression()
    var cases = {}
    var default_case

    self._consume(LBRACE, "'{' expected after using expression")
    self._ignore_newline()

    var state = 0

    while !self._match(RBRACE) and !self._check(EOF) {
      if self._match(WHEN, DEFAULT, COMMENT, DOC, NEWLINE) {
        if state == 1 
          die ParseException(self._previous(), "'when' cannot exist after a default")

        if [DOC, COMMENT, NEWLINE].contains(self._previous().type) {}
        else if self._previous().type == WHEN {
          var tmp_cases = []
          do {
            tmp_cases.append(self._expression())
          } while self._match(COMMA)
          var stmt = self._statement()

          for tmp in tmp_cases {
            cases[tmp] = stmt
          }
        } else {
          state = 1
          default_case = self._statement()
        }
      } else {
        die ParseException(self._previous(), 'Invalid using statement')
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

    while !self._match(NEWLINE, EOF, LBRACE) {
      self._advance()
      path.append(self._previous().literal)
    }

    if self._previous().type == LBRACE {
      var scan = true
      while !self._check(RBRACE) and scan {
        self._ignore_newline()
        elements.append(self._consume_any('identifier expected', IDENTIFIER, MULTIPLY).literal)
        if !self._match(COMMA)
          scan = false
        self._ignore_newline()
      }

      self._consume(RBRACE, "'}' expected at end of selective import")
    }

    return ImportStmt(''.join(path), elements)
  }

  /**
   * try...catch...finally... blocks
   */
  _try() {
    self._consume(LBRACE, "'{' expected after try")
    var body = self._block()
    var exception_type, exception_var, catch_body, finally_body
    var has_catch = false, has_finally = false

    if self._match(CATCH) {
      self._consume(IDENTIFIER, 'exception name expected')
      exception_type = self._previous().literal

      if self._check(IDENTIFIER) {
        self._consume(IDENTIFIER, 'exception variable expected')
        exception_var = self._previous().literal
      }

      self._consume(LBRACE, "'{' expected after catch expression")
      catch_body = self._block()
      has_catch = true
    }
    
    if self._match(FINALLY) {
      has_finally = true
      self._consume(LBRACE, "'{' expected after finally")
      finally_body = self._block()
    }

    if !has_catch and !has_finally
      die ParseException(self._previous(), 'invalid try statement')

    var catch_stmt, finally_stmt
    if has_catch catch_stmt = CatchStmt(exception_type, exception_var, catch_body)
    if has_finally finally_stmt = FinallyStmt(finally_body)

    return TryStmt(body, catch_stmt, finally_stmt)
  }

  /**
   * iter loops
   */
  _iter() {
    var decl
    if !self._check(SEMICOLON) {
      if self._check(VAR) {
        self._consume(VAR, 'variable declaration expected')
      }
      decl = self._var()
    }
    self._consume(SEMICOLON, "';' expected")
    self._ignore_newline()

    var condition
    if !self._check(SEMICOLON) condition = self._expression()
    self._consume(SEMICOLON, "';' expected")
    self._ignore_newline()

    var iterator
    if !self._check(LBRACE) iterator = self._expr_stmt(true)
    self._ignore_newline()

    var body = self._statement()
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
    } else if self._match(DO) {
      result = self._do_while()
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
    } else if self._match(COMMENT) {
      result = CommentStmt(self._previous().literal[1,].trim())
    } else if self._match(DOC) {
      result = DocDefn(self._previous().literal[2,-2].trim())
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
    self._consume(IDENTIFIER, 'variable name expected')
    var result = self._previous().literal

    if self._match(EQUAL)
      result = VarDecl(result, self._expression())
    else result = VarDecl(result, nil)

    if self._check(COMMA) {
      result = [result] # we want to return an array of declarations

      while self._match(COMMA) {
        self._ignore_newline()
        self._consume(IDENTIFIER, 'variable name expected')
        var r = self._previous().literal

        if self._match(EQUAL)
          r = VarDecl(r, self._expression())
        else r = VarDecl(r, nil)

        result.append(r)
      }
    }

    return result
  }

  /**
   * anonymous compartibility functions
   */
  _anonymous_compat() {
    var params = []

    while !self._check(BAR) {
      params.append(self._consume_any('parameter name expected', IDENTIFIER, TRI_DOT).literal)

      if !self._check(BAR)
        self._consume(COMMA, "',' expected between function params")
    }

    self._consume(BAR, "'|' expected after anonymous function args")
    self._consume(LBRACE, "'{' expected after function declaration")
    var body = self._block()

    return FunctionDecl('', params, body)
  }

  /**
   * anonymous functions
   */
  _anonymous() {
    var params = []
    self._consume(LPAREN, "expected '(' at start of anonymous function")

    while !self._check(RPAREN) {
      params.append(self._consume_any('parameter name expected', IDENTIFIER, TRI_DOT).literal)

      if !self._check(RPAREN)
        self._consume(COMMA, "',' expected between function params")
    }

    self._consume(RPAREN, "expected ')' after anonymous function parameters")
    self._consume(LBRACE, "'{' expected after function declaration")
    var body = self._block()

    return FunctionDecl('', params, body)
  }

  /**
   * function definitions
   */
  _def() {
    self._consume(IDENTIFIER, 'function name expected')
    var name = self._previous().literal
    var params = []

    self._consume(LPAREN, "'(' expected after function name")
    while self._match(IDENTIFIER, TRI_DOT) {
      params.append(self._previous().literal)

      if !self._check(RPAREN) {
        self._consume(COMMA, "',' expected between function arguments")
        self._ignore_newline()
      }
    }
    self._consume(RPAREN, "')' expected after function arguments")
    self._consume(LBRACE, "'{' expected after function declaration")
    var body = self._block()

    return FunctionDecl(name, params, body)
  }

  /**
   * class fields
   */
  _class_field(is_static) {
    self._consume(IDENTIFIER, 'class property name expected')
    var name = self._previous().literal, value

    if self._match(EQUAL) value = self._expression()
    self._end_statement()
    self._ignore_newline()

    return PropertyDecl(name, value, is_static)
  }

  /**
   * class methods
   */
  _method(is_static) {
    self._consume_any('method name expected', IDENTIFIER, DECORATOR)
    var name = self._previous().literal
    var params = []

    self._consume(LPAREN, "'(' expected after method name")
    while self._match(IDENTIFIER, TRI_DOT) {
      params.append(self._previous().literal)

      if !self._check(RPAREN) {
        self._consume(COMMA, "',' expected between method arguments")
        self._ignore_newline()
      }
    }
    self._consume(RPAREN, "'(' expected after method arguments")
    self._consume(LBRACE, "'{' expected after method declaration")
    var body = self._block()

    return MethodDecl(name, params, body, is_static)
  }

  /**
   * classes
   */
  _class() {
    var properties = [], methods = []

    self._consume(IDENTIFIER, 'class name expected')
    var name = self._previous().literal, superclass

    if self._match(LESS) {
      self._consume(IDENTIFIER, 'super class name expected')
      superclass = self._previous().literal
    }

    self._ignore_newline()
    self._consume(LBRACE, "'{' expected after class declaration")
    self._ignore_newline()

    while !self._check(RBRACE) and !self._check(EOF) {
      var is_static = false
      var doc

      self._ignore_newline()
      while self._match(COMMENT) {}
      self._ignore_newline()

      if self._match(DOC)
        doc = DocDefn(self._previous().literal[2,-2].trim())

      self._ignore_newline()

      if self._match(STATIC) is_static = true

      if self._match(VAR) {
        var prop = self._class_field(is_static)
        if doc and !prop.name.starts_with('_') 
          prop.doc = _get_doc_string(doc.data)
        properties.append(prop)
      } else {
        var method = self._method(is_static)
        if doc and !method.name.starts_with('_') and !method.name.starts_with('@') 
          method.doc = _get_doc_string(doc.data)
        methods.append(method)
        self._ignore_newline()
      }
    }

    self._consume(RBRACE, "'{' expected at end of class definition")

    return ClassDecl(name, superclass, properties, methods)
  }

  /**
   * Blade's declarations
   */
  _declaration() {
    self._ignore_newline()
    
    var result

    if self._match(VAR) {
      result = self._var()
    } else if self._match(DEF) {
      result = self._def()
    } else if self._match(CLASS) {
      result = self._class()
    } else if self._match(COMMENT) {
      result = CommentStmt(self._previous().literal[1,].trim())
    } else if self._match(DOC) {
      result = DocDefn(self._previous().literal[2,-2].trim())
    } else if self._match(LBRACE) {
      if !self._check(NEWLINE) and self._block_count == 0 
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
   * parse()
   * 
   * parses the raw source tokens passed into relevant class and
   * outputs a stream of AST objects that can be one of
   * Expr (expressions), Stmt (statements) or Decl (declarations)
   * @return ParseResult
   */
  parse() {
    var declarations = ParseResult()

    while !self._is_at_end()
      declarations.append(self._declaration())

    return declarations
  }

  @to_string() {
    return '<ast::Parser>'
  }
}