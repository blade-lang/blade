#!-- This file is autogenerated by scripts/ast.b
/**
 * @class Defn
 * base Defn class
 */
class Defn {
}

/**
 * @class DocDefn
 */
class DocDefn < Defn {

  /**
   * @constructor Doc
   */
  DocDefn(data) {
    self.data = data
  }

  @to_json() {
    return {
      data: self.data,
    }
  }
}

