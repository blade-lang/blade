/**
 * @class Stmt
 * base Stmt class
 */
class Stmt {}

/**
 * @class Echo
 */
class EchoStmt < Stmt {

  /**
   * @constructor Echo
   */
  EchoStmt(value) {
    self.value = value
  }
}

/**
 * @class Expr
 */
class ExprStmt < Stmt {

  /**
   * @constructor Expr
   */
  ExprStmt(expr) {
    self.expr = expr
  }
}

/**
 * @class If
 */
class IfStmt < Stmt {

  /**
   * @constructor If
   */
  IfStmt(condition, truth, falsy) {
    self.condition = condition
    self.truth = truth
    self.falsy = falsy
  }
}

/**
 * @class Iter
 */
class IterStmt < Stmt {

  /**
   * @constructor Iter
   */
  IterStmt(declaration, condition, iterator, body) {
    self.declaration = declaration
    self.condition = condition
    self.iterator = iterator
    self.body = body
  }
}

/**
 * @class While
 */
class WhileStmt < Stmt {

  /**
   * @constructor While
   */
  WhileStmt(condition, body) {
    self.condition = condition
    self.body = body
  }
}

/**
 * @class For
 */
class ForStmt < Stmt {

  /**
   * @constructor For
   */
  ForStmt(vars, iterable, body) {
    self.vars = vars
    self.iterable = iterable
    self.body = body
  }
}

/**
 * @class Continue
 */
class ContinueStmt < Stmt {

}

/**
 * @class Break
 */
class BreakStmt < Stmt {

}

/**
 * @class Die
 */
class DieStmt < Stmt {

  /**
   * @constructor Die
   */
  DieStmt(exception) {
    self.exception = exception
  }
}

/**
 * @class Return
 */
class ReturnStmt < Stmt {

  /**
   * @constructor Return
   */
  ReturnStmt(value) {
    self.value = value
  }
}

/**
 * @class Assert
 */
class AssertStmt < Stmt {

  /**
   * @constructor Assert
   */
  AssertStmt(expr, message) {
    self.expr = expr
    self.message = message
  }
}

/**
 * @class Using
 */
class UsingStmt < Stmt {

  /**
   * @constructor Using
   */
  UsingStmt(expr, cases, default_case) {
    self.expr = expr
    self.cases = cases
    self.default_case = default_case
  }
}

/**
 * @class Import
 */
class ImportStmt < Stmt {

  /**
   * @constructor Import
   */
  ImportStmt(path) {
    self.path = path
  }
}

/**
 * @class Catch
 */
class CatchStmt < Stmt {

  /**
   * @constructor Catch
   */
  CatchStmt(type, var_name, body) {
    self.type = type
    self.var_name = var_name
    self.body = body
  }
}

/**
 * @class Finally
 */
class FinallyStmt < Stmt {

  /**
   * @constructor Finally
   */
  FinallyStmt(body) {
    self.body = body
  }
}

/**
 * @class Try
 */
class TryStmt < Stmt {

  /**
   * @constructor Try
   */
  TryStmt(body, catch_stmt, finally_stmt) {
    self.body = body
    self.catch_stmt = catch_stmt
    self.finally_stmt = finally_stmt
  }
}

