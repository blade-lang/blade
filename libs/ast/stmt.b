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

