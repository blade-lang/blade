#!-- This file is autogenerated by scripts/ast.b
/**
 * @class Expr
 * base Expr class
 */
class Expr {}

/**
 * @class BinaryExpr
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

  @to_json() {
    return {
      left: self.left,
      op: self.op,
      right: self.right,
    }
  }
}

/**
 * @class GroupExpr
 */
class GroupExpr < Expr {

  /**
   * @constructor Group
   */
  GroupExpr(expression) {
    self.expression = expression
  }

  @to_json() {
    return {
      expression: self.expression,
    }
  }
}

/**
 * @class LiteralExpr
 */
class LiteralExpr < Expr {

  /**
   * @constructor Literal
   */
  LiteralExpr(value) {
    self.value = value
  }

  @to_json() {
    return {
      value: self.value,
    }
  }
}

/**
 * @class IdentifierExpr
 */
class IdentifierExpr < Expr {

  /**
   * @constructor Identifier
   */
  IdentifierExpr(value) {
    self.value = value
  }

  @to_json() {
    return {
      value: self.value,
    }
  }
}

/**
 * @class UnaryExpr
 */
class UnaryExpr < Expr {

  /**
   * @constructor Unary
   */
  UnaryExpr(op, right) {
    self.op = op
    self.right = right
  }

  @to_json() {
    return {
      op: self.op,
      right: self.right,
    }
  }
}

/**
 * @class ConditionExpr
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

  @to_json() {
    return {
      expr: self.expr,
      truth: self.truth,
      falsy: self.falsy,
    }
  }
}

/**
 * @class CallExpr
 */
class CallExpr < Expr {

  /**
   * @constructor Call
   */
  CallExpr(callee, args) {
    self.callee = callee
    self.args = args
  }

  @to_json() {
    return {
      callee: self.callee,
      args: self.args,
    }
  }
}

/**
 * @class GetExpr
 */
class GetExpr < Expr {

  /**
   * @constructor Get
   */
  GetExpr(expr, name) {
    self.expr = expr
    self.name = name
  }

  @to_json() {
    return {
      expr: self.expr,
      name: self.name,
    }
  }
}

/**
 * @class SetExpr
 */
class SetExpr < Expr {

  /**
   * @constructor Set
   */
  SetExpr(expr, name, value) {
    self.expr = expr
    self.name = name
    self.value = value
  }

  @to_json() {
    return {
      expr: self.expr,
      name: self.name,
      value: self.value,
    }
  }
}

/**
 * @class IndexExpr
 */
class IndexExpr < Expr {

  /**
   * @constructor Index
   */
  IndexExpr(args) {
    self.args = args
  }

  @to_json() {
    return {
      args: self.args,
    }
  }
}

/**
 * @class ListExpr
 */
class ListExpr < Expr {

  /**
   * @constructor List
   */
  ListExpr(items) {
    self.items = items
  }

  @to_json() {
    return {
      items: self.items,
    }
  }
}

/**
 * @class DictExpr
 */
class DictExpr < Expr {

  /**
   * @constructor Dict
   */
  DictExpr(keys, values) {
    self.keys = keys
    self.values = values
  }

  @to_json() {
    return {
      keys: self.keys,
      values: self.values,
    }
  }
}

/**
 * @class InterpolationExpr
 */
class InterpolationExpr < Expr {

  /**
   * @constructor Interpolation
   */
  InterpolationExpr(data) {
    self.data = data
  }

  @to_json() {
    return {
      data: self.data,
    }
  }
}

