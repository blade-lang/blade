#!-- This file is autogenerated by scripts/ast.b
/**
 * base Stmt class
 */
class Stmt {
}

/**
 * Echo Stmt representation.
 * 
 * @serializable
 */
class EchoStmt < Stmt {

  /**
   * @param Stmt|any|nil value
   * @constructor
   */
  EchoStmt(value) {
    self.value = value
  }

  @to_json() {
    return {
      type: 'EchoStmt',
      value: self.value,
    }
  }
}

/**
 * Expr Stmt representation.
 * 
 * @serializable
 */
class ExprStmt < Stmt {

  /**
   * @param Stmt|any|nil expr
   * @constructor
   */
  ExprStmt(expr) {
    self.expr = expr
  }

  @to_json() {
    return {
      type: 'ExprStmt',
      expr: self.expr,
    }
  }
}

/**
 * If Stmt representation.
 * 
 * @serializable
 */
class IfStmt < Stmt {

  /**
   * @param Stmt|any|nil condition
   * @param Stmt|any|nil truth
   * @param Stmt|any|nil falsy
   * @constructor
   */
  IfStmt(condition, truth, falsy) {
    self.condition = condition
    self.truth = truth
    self.falsy = falsy
  }

  @to_json() {
    return {
      type: 'IfStmt',
      condition: self.condition,
      truth: self.truth,
      falsy: self.falsy,
    }
  }
}

/**
 * Iter Stmt representation.
 * 
 * @serializable
 */
class IterStmt < Stmt {

  /**
   * @param Stmt|any|nil declaration
   * @param Stmt|any|nil condition
   * @param Stmt|any|nil iterator
   * @param Stmt|any|nil body
   * @constructor
   */
  IterStmt(declaration, condition, iterator, body) {
    self.declaration = declaration
    self.condition = condition
    self.iterator = iterator
    self.body = body
  }

  @to_json() {
    return {
      type: 'IterStmt',
      declaration: self.declaration,
      condition: self.condition,
      iterator: self.iterator,
      body: self.body,
    }
  }
}

/**
 * While Stmt representation.
 * 
 * @serializable
 */
class WhileStmt < Stmt {

  /**
   * @param Stmt|any|nil condition
   * @param Stmt|any|nil body
   * @constructor
   */
  WhileStmt(condition, body) {
    self.condition = condition
    self.body = body
  }

  @to_json() {
    return {
      type: 'WhileStmt',
      condition: self.condition,
      body: self.body,
    }
  }
}

/**
 * DoWhile Stmt representation.
 * 
 * @serializable
 */
class DoWhileStmt < Stmt {

  /**
   * @param Stmt|any|nil body
   * @param Stmt|any|nil condition
   * @constructor
   */
  DoWhileStmt(body, condition) {
    self.body = body
    self.condition = condition
  }

  @to_json() {
    return {
      type: 'DoWhileStmt',
      body: self.body,
      condition: self.condition,
    }
  }
}

/**
 * For Stmt representation.
 * 
 * @serializable
 */
class ForStmt < Stmt {

  /**
   * @param Stmt|any|nil vars
   * @param Stmt|any|nil iterable
   * @param Stmt|any|nil body
   * @constructor
   */
  ForStmt(vars, iterable, body) {
    self.vars = vars
    self.iterable = iterable
    self.body = body
  }

  @to_json() {
    return {
      type: 'ForStmt',
      vars: self.vars,
      iterable: self.iterable,
      body: self.body,
    }
  }
}

/**
 * Continue Stmt representation.
 * 
 * @serializable
 */
class ContinueStmt < Stmt {

  @to_json() {
    return {
      type: 'ContinueStmt',
     }
  }
}

/**
 * Break Stmt representation.
 * 
 * @serializable
 */
class BreakStmt < Stmt {

  @to_json() {
    return {
      type: 'BreakStmt',
     }
  }
}

/**
 * Raise Stmt representation.
 * 
 * @serializable
 */
class RaiseStmt < Stmt {

  /**
   * @param Stmt|any|nil exception
   * @constructor
   */
  RaiseStmt(exception) {
    self.exception = exception
  }

  @to_json() {
    return {
      type: 'RaiseStmt',
      exception: self.exception,
    }
  }
}

/**
 * Return Stmt representation.
 * 
 * @serializable
 */
class ReturnStmt < Stmt {

  /**
   * @param Stmt|any|nil value
   * @constructor
   */
  ReturnStmt(value) {
    self.value = value
  }

  @to_json() {
    return {
      type: 'ReturnStmt',
      value: self.value,
    }
  }
}

/**
 * Assert Stmt representation.
 * 
 * @serializable
 */
class AssertStmt < Stmt {

  /**
   * @param Stmt|any|nil expr
   * @param Stmt|any|nil message
   * @constructor
   */
  AssertStmt(expr, message) {
    self.expr = expr
    self.message = message
  }

  @to_json() {
    return {
      type: 'AssertStmt',
      expr: self.expr,
      message: self.message,
    }
  }
}

/**
 * Using Stmt representation.
 * 
 * @serializable
 */
class UsingStmt < Stmt {

  /**
   * @param Stmt|any|nil expr
   * @param Stmt|any|nil cases
   * @param Stmt|any|nil default_case
   * @constructor
   */
  UsingStmt(expr, cases, default_case) {
    self.expr = expr
    self.cases = cases
    self.default_case = default_case
  }

  @to_json() {
    return {
      type: 'UsingStmt',
      expr: self.expr,
      cases: self.cases,
      default_case: self.default_case,
    }
  }
}

/**
 * Import Stmt representation.
 * 
 * @serializable
 */
class ImportStmt < Stmt {

  /**
   * @param Stmt|any|nil path
   * @param Stmt|any|nil elements
   * @constructor
   */
  ImportStmt(path, elements) {
    self.path = path
    self.elements = elements
  }

  @to_json() {
    return {
      type: 'ImportStmt',
      path: self.path,
      elements: self.elements,
    }
  }
}

/**
 * Catch Stmt representation.
 * 
 * @serializable
 */
class CatchStmt < Stmt {

  /**
   * @param Stmt|any|nil body
   * @param Stmt|any|nil var_name
   * @constructor
   */
  CatchStmt(body, var_name) {
    self.body = body
    self.var_name = var_name
  }

  @to_json() {
    return {
      type: 'CatchStmt',
      body: self.body,
      var_name: self.var_name,
    }
  }
}

/**
 * Comment Stmt representation.
 * 
 * @serializable
 */
class CommentStmt < Stmt {

  /**
   * @param Stmt|any|nil data
   * @constructor
   */
  CommentStmt(data) {
    self.data = data
  }

  @to_json() {
    return {
      type: 'CommentStmt',
      data: self.data,
    }
  }
}

/**
 * Block Stmt representation.
 * 
 * @serializable
 */
class BlockStmt < Stmt {

  /**
   * @param Stmt|any|nil body
   * @constructor
   */
  BlockStmt(body) {
    self.body = body
  }

  @to_json() {
    return {
      type: 'BlockStmt',
      body: self.body,
    }
  }
}

/**
 * Assign Stmt representation.
 * 
 * @serializable
 */
class AssignStmt < Stmt {

  /**
   * @param Stmt|any|nil expr
   * @param Stmt|any|nil type
   * @param Stmt|any|nil value
   * @constructor
   */
  AssignStmt(expr, type, value) {
    self.expr = expr
    self.type = type
    self.value = value
  }

  @to_json() {
    return {
      type: 'AssignStmt',
      expr: self.expr,
      type: self.type,
      value: self.value,
    }
  }
}

