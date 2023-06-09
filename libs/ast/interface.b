#!-- part of the ast module

# import .defn { DocDefn }
# import .decl { Decl }

import .decl
import .defn

/**
 * Represents the result of an ast parse operation
 * @serializable
 * @iterable
 */
class ParseResult {
  /**
   * holds the result set
   */
  var _results = []

  /**
   * append(item: any)
   * 
   * adds a new item to the parse result
   */
  append(item) {
    if self._results.length() > 0 {
      if instance_of(self._results.last(), defn.DocDefn) and
        instance_of(item, decl.Decl) {
          item.doc = '${self._results.pop().data}\n'.replace('/^\s*[*]\s?(.*)\\n/m', '$1\n').trim()
      }
    }
    self._results.append(item)
  }

  /**
   * length()
   * 
   * Returns the length of items in the parsed result.
   * @returns number
   */
  length() {
    return self._results.length()
  }

  /**
   * get(index: int)
   * 
   * Returns the item at the given ParseResult index or throws exception if out of range.
   * @returns Ast
   */
  get(index) {
    if index >= 0 and index < self.length()
      return self._results[index]
    die Exception('ParseResult index ${index} out of range')
  }

  /**
   * to_list()
   * 
   * Returns the items in the ParseResult as a list object.
   * @returns List<Ast>
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
      die Exception('ParseResult is numerically indexed')
    if index < self._results.length() - 1 return index + 1
    return nil
  }

  @to_json() {
    return self._results
  }

  @to_string() {
    return '<ast::ParseResult>'
  }
}