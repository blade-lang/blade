/**
 * @class Expr
 * base Expr class
 */
class Expr {}

/**
 * @class Binary
 */
class Binary < Expr {

  /**
   * @constructor Binary
   */
  Binary(left, op, right) {
    self.left = left
    self.op = op
    self.right = right
  }
}

/**
 * @class Group
 */
class Group < Expr {

  /**
   * @constructor Group
   */
  Group(expression) {
    self.expression = expression
  }
}

/**
 * @class Literal
 */
class Literal < Expr {

  /**
   * @constructor Literal
   */
  Literal(value) {
    self.value = value
  }
}

/**
 * @class Unary
 */
class Unary < Expr {

  /**
   * @constructor Unary
   */
  Unary(op, right) {
    self.op = op
    self.right = right
  }
}

/**
 * @class Condition
 */
class Condition < Expr {

  /**
   * @constructor Condition
   */
  Condition(expr, truth, falsy) {
    self.expr = expr
    self.truth = truth
    self.falsy = falsy
  }
}

