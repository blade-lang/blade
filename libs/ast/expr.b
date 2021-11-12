/**
 * @class Expr
 * base Expr class
 */
class Expr {}

/**
 * @class Binary
 */
class BinaryExpr < Expr {

  /**
   * @constructor Binary
   */
  BinaryExpr(left, op, right) {
    self.left = left
    self.op = op
    self.right = right
  }
}

/**
 * @class Group
 */
class GroupExpr < Expr {

  /**
   * @constructor Group
   */
  GroupExpr(expression) {
    self.expression = expression
  }
}

/**
 * @class Literal
 */
class LiteralExpr < Expr {

  /**
   * @constructor Literal
   */
  LiteralExpr(value) {
    self.value = value
  }
}

/**
 * @class Identifier
 */
class IdentifierExpr < Expr {

  /**
   * @constructor Identifier
   */
  IdentifierExpr(value) {
    self.value = value
  }
}

/**
 * @class Unary
 */
class UnaryExpr < Expr {

  /**
   * @constructor Unary
   */
  UnaryExpr(op, right) {
    self.op = op
    self.right = right
  }
}

/**
 * @class Condition
 */
class ConditionExpr < Expr {

  /**
   * @constructor Condition
   */
  ConditionExpr(expr, truth, falsy) {
    self.expr = expr
    self.truth = truth
    self.falsy = falsy
  }
}

