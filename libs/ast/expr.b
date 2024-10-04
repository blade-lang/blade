#!-- This file is autogenerated by scripts/ast.b
/**
 * base Expr class
 */
class Expr {
}

/**
 * Binary Expr representation.
 * 
 * @serializable
 */
class BinaryExpr < Expr {

  /**
   * @param Expr|any|nil left
   * @param Expr|any|nil op
   * @param Expr|any|nil right
   * @constructor
   */
  BinaryExpr(left, op, right) {
    self.left = left
    self.op = op
    self.right = right
  }

  @to_json() {
    return {
      type: 'BinaryExpr',
      left: self.left,
      op: self.op,
      right: self.right,
    }
  }
}

/**
 * Group Expr representation.
 * 
 * @serializable
 */
class GroupExpr < Expr {

  /**
   * @param Expr|any|nil expression
   * @constructor
   */
  GroupExpr(expression) {
    self.expression = expression
  }

  @to_json() {
    return {
      type: 'GroupExpr',
      expression: self.expression,
    }
  }
}

/**
 * Literal Expr representation.
 * 
 * @serializable
 */
class LiteralExpr < Expr {

  /**
   * @param Expr|any|nil value
   * @constructor
   */
  LiteralExpr(value) {
    self.value = value
  }

  @to_json() {
    return {
      type: 'LiteralExpr',
      value: self.value,
    }
  }
}

/**
 * Identifier Expr representation.
 * 
 * @serializable
 */
class IdentifierExpr < Expr {

  /**
   * @param Expr|any|nil value
   * @constructor
   */
  IdentifierExpr(value) {
    self.value = value
  }

  @to_json() {
    return {
      type: 'IdentifierExpr',
      value: self.value,
    }
  }
}

/**
 * Unary Expr representation.
 * 
 * @serializable
 */
class UnaryExpr < Expr {

  /**
   * @param Expr|any|nil op
   * @param Expr|any|nil right
   * @constructor
   */
  UnaryExpr(op, right) {
    self.op = op
    self.right = right
  }

  @to_json() {
    return {
      type: 'UnaryExpr',
      op: self.op,
      right: self.right,
    }
  }
}

/**
 * Condition Expr representation.
 * 
 * @serializable
 */
class ConditionExpr < Expr {

  /**
   * @param Expr|any|nil expr
   * @param Expr|any|nil truth
   * @param Expr|any|nil falsy
   * @constructor
   */
  ConditionExpr(expr, truth, falsy) {
    self.expr = expr
    self.truth = truth
    self.falsy = falsy
  }

  @to_json() {
    return {
      type: 'ConditionExpr',
      expr: self.expr,
      truth: self.truth,
      falsy: self.falsy,
    }
  }
}

/**
 * Call Expr representation.
 * 
 * @serializable
 */
class CallExpr < Expr {

  /**
   * @param Expr|any|nil callee
   * @param Expr|any|nil args
   * @constructor
   */
  CallExpr(callee, args) {
    self.callee = callee
    self.args = args
  }

  @to_json() {
    return {
      type: 'CallExpr',
      callee: self.callee,
      args: self.args,
    }
  }
}

/**
 * Get Expr representation.
 * 
 * @serializable
 */
class GetExpr < Expr {

  /**
   * @param Expr|any|nil expr
   * @param Expr|any|nil name
   * @constructor
   */
  GetExpr(expr, name) {
    self.expr = expr
    self.name = name
  }

  @to_json() {
    return {
      type: 'GetExpr',
      expr: self.expr,
      name: self.name,
    }
  }
}

/**
 * Set Expr representation.
 * 
 * @serializable
 */
class SetExpr < Expr {

  /**
   * @param Expr|any|nil expr
   * @param Expr|any|nil name
   * @param Expr|any|nil value
   * @constructor
   */
  SetExpr(expr, name, value) {
    self.expr = expr
    self.name = name
    self.value = value
  }

  @to_json() {
    return {
      type: 'SetExpr',
      expr: self.expr,
      name: self.name,
      value: self.value,
    }
  }
}

/**
 * Index Expr representation.
 * 
 * @serializable
 */
class IndexExpr < Expr {

  /**
   * @param Expr|any|nil args
   * @constructor
   */
  IndexExpr(args) {
    self.args = args
  }

  @to_json() {
    return {
      type: 'IndexExpr',
      args: self.args,
    }
  }
}

/**
 * List Expr representation.
 * 
 * @serializable
 */
class ListExpr < Expr {

  /**
   * @param Expr|any|nil items
   * @constructor
   */
  ListExpr(items) {
    self.items = items
  }

  @to_json() {
    return {
      type: 'ListExpr',
      items: self.items,
    }
  }
}

/**
 * Dict Expr representation.
 * 
 * @serializable
 */
class DictExpr < Expr {

  /**
   * @param Expr|any|nil keys
   * @param Expr|any|nil values
   * @constructor
   */
  DictExpr(keys, values) {
    self.keys = keys
    self.values = values
  }

  @to_json() {
    return {
      type: 'DictExpr',
      keys: self.keys,
      values: self.values,
    }
  }
}

/**
 * Interpolation Expr representation.
 * 
 * @serializable
 */
class InterpolationExpr < Expr {

  /**
   * @param Expr|any|nil data
   * @constructor
   */
  InterpolationExpr(data) {
    self.data = data
  }

  @to_json() {
    return {
      type: 'InterpolationExpr',
      data: self.data,
    }
  }
}

