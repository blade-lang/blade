#!-- part of the ast module

# import .defn { DocDefn }
# import .decl { Decl }

import .decl
import .defn

/**
 * Represents the result of an ast parse operation
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
          item.doc = '${self._results.pop().data}\n'.replace('/\\s*\\*(.*)\n/', '$1\n ').trim()
      }
    }
    self._results.append(item)
  }

  /**
   * to_list() override decorator
   */
  @to_list() {
    return self._results
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
}