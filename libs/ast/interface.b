#!-- part of the ast module

# import .defn { DocDefn }
# import .decl { Decl }

import .decl
import .defn

/**
 * Represents the result of an ast parse operation.
 * 
 * @printable
 * @serializable
 * @iterable
 */
class ParseResult {
  /**
   * holds the result set
   */
  var _results = []

  /**
   * @param string path
   * @constructor
   */
  ParseResult(path) {
    self._path = path
  }

  /**
   * Adds a new item to the parse result
   * 
   * @param Expr|Decl|Defn|Stmt item
   */
  append(item) {
    if self._results.length() > 0 {
      if instance_of(self._results.last(), defn.DocDefn) and
        instance_of(item, decl.Decl) {
          item.doc = self._results.pop().data
      }
    }
    self._results.append(item)
  }

  /**
   * Returns the length of items in the parsed result.
   * 
   * @returns number
   */
  length() {
    return self._results.length()
  }

  /**
   * Returns the item at the given ParseResult index or throws exception if out of range.
   * 
   * @param int index
   * @returns Expr|Decl|Defn|Stmt
   */
  get(index) {
    if index >= 0 and index < self.length()
      return self._results[index]
    raise RangeError('ParseResult index ${index} out of range')
  }

  /**
   * Returns the items in the ParseResult as a list object.
   * 
   * @returns list[Expr|Decl|Defn|Stmt]
   */
  to_list() {
    return self._results
  }

  /**
   * to_list() override decorator
   */
  @to_list() {
    return self.to_list()
  }

  /**
   * iterator decoration
   */
  @iter(index) {
    return self._results[index]
  }

  /**
   * iterator decoration
   */
  @itern(index) {
    if index == nil return 0
    if !is_number(index)
      raise ArgumentError('ParseResult is numerically indexed')
    if index < self._results.length() - 1 return index + 1
    return nil
  }

  @to_json() {
    return self._results
  }

  @to_string() {
    return '<ast::ParseResult(${self._path}){${self._results.length()} results}>'
  }
}