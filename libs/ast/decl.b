/**
 * @class Decl
 * base Decl class
 */
class Decl {}

/**
 * @class Var
 */
class VarDecl < Decl {

  /**
   * @constructor Var
   */
  VarDecl(name, value) {
    self.name = name
    self.value = value
  }
}

/**
 * @class Function
 */
class FunctionDecl < Decl {

  /**
   * @constructor Function
   */
  FunctionDecl(name, params, body) {
    self.name = name
    self.params = params
    self.body = body
  }
}

