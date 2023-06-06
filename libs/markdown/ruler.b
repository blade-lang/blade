import iters

/**
 * Helper class, used by [[markdown#core]], [[markdown#block]] and
 * [[markdown#inline]] to manage sequences of functions (rules):
 *
 * - keep rules in defined order
 * - assign the name to each rule
 * - enable/disable rules
 * - add/replace rules
 * - allow assign rules to additional named chains (in the same)
 * - cacheing lists of active rules
 *
 * You will not need use this class directly until write plugins. For simple
 * rules control use [[markdown.disable]], [[markdown.enable]] and
 * [[markdown.use]].
 */
class Ruler {
  # List of added rules. Each element is:
  #
  # {
  #   name: XXX,
  #   enabled: Boolean,
  #   fn: Function(),
  #   alt: [ name2, name3 ]
  # }
  var __rules__ = []

  # Cached rule chains.
  #
  # First level - chain name, '' for default.
  # Second level - diginal anchor for fast filtering by charcodes.
  var __cache__ = []

  __find__(name) {
    iter var i = 0; i < self.__rules__.length(); i++ {
      if self.__rules__[i].name == name {
        return i
      }
    }
    return -1
  }

  __compile__() {
    var chains = [ '' ]
  
    # collect unique names
    iters.each(self.__rules__, @(rule) {
      if !rule.enabled return
  
      iters.each(rule.alt, @(alt_name) {
        if chains.index_of(alt_name) < 0 {
          chains.append(alt_name)
        }
      })
    })
  
    self.__cache__ = {}
  
    iters.each(chains, @(chain) {
      self.__cache__[chain] = []
      iters.each(self.__rules__, @(rule) {
        if !rule.enabled return
  
        if chain and rule.alt.index_of(chain) < 0 return
  
        self.__cache__[chain].append(rule.fn)
      })
    })
  }

  /**
   * Replace rule by name with new function & options. Dies error if name not
   * found.
   *
   * ##### Options:
   *
   * - __alt__ - list with names of "alternate" chains.
   *
   * ##### Example
   *
   * Replace existing typographer replacement rule with new one:
   *
   * ```blade
   * import markdown as md
   *
   * md.core.ruler.at('replacements', @(state) {
   *   #...
   * })
   * ```
   * 
   * @param {string} name: rule name to replace.
   * @param {function} fn: new rule function.
   * @param {dict|nil} options: new rule options (optional).
   */
  at(name, fn, options) {
    var index = self.__find__(name)
    var opt = options or {}
  
    if index == -1 die Exception('Parser rule not found: ' + name)
  
    self.__rules__[index].fn = fn
    self.__rules__[index].alt = opt.alt or []
    self.__cache__ = nil
  }

  /**
   * Add new rule to chain before one with given name. See also
   * [[Ruler.after]], [[Ruler.push]].
   *
   * ##### Options:
   *
   * - __alt__ - list with names of "alternate" chains.
   *
   * ##### Example
   *
   * ```blade
   * import markdown as md
   *
   * md.block.ruler.before('paragraph', 'my_rule', @(state) {
   *   #...
   * })
   * ```
   * 
   * @param {string} before_name: new rule will be added before this one.
   * @param {string} rule_name: name of added rule.
   * @param {function} fn: rule function.
   * @param {dict|nil} options: rule options (optional).
   */
  before(before_name, rule_name, fn, options) {
    var index = self.__find__(before_name)
    var opt = options or {}
  
    if index == -1 die Exception('Parser rule not found: ' + before_name)
  
    self.__rules__.remove_at(index)
    self.__rules__.insert(index, {
      name: rule_name,
      enabled: true,
      fn: fn,
      alt: opt.alt or []
    })
  
    self.__cache__ = nil
  }

  /**
   * Add new rule to chain after one with given name. See also
   * [[Ruler.before]], [[Ruler.push]].
   *
   * ##### Options:
   *
   * - __alt__ - list with names of "alternate" chains.
   *
   * ##### Example
   *
   * ```blade
   * import markdown as md
   *
   * md.inline.ruler.after('text', 'my_rule', @(state) {
   *   #...
   * })
   * ```
   * 
   * @param {string} after_name: new rule will be added after this one.
   * @param {string} rule_name: name of added rule.
   * @param {function} fn: rule function.
   * @param {dict|nil} options: rule options (optional).
   */
  after(after_name, rule_name, fn, options) {
    var index = self.__find__(after_name)
    var opt = options or {}
  
    if index == -1 die Exception('Parser rule not found: ' + after_name)
  
    self.__rules__.remove_at(index + 1)
    self.__rules__.insert(index + 1, {
      name: rule_name,
      enabled: true,
      fn: fn,
      alt: opt.alt or []
    })
  
    self.__cache__ = nil
  }

  /**
   * Push new rule to the end of chain. See also
   * [[Ruler.before]], [[Ruler.after]].
   *
   * ##### Options:
   *
   * - __alt__ - list with names of "alternate" chains.
   *
   * ##### Example
   *
   * ```blade
   * import markdown as md
   *
   * md.core.ruler.push('my_rule', @(state) {
   *   #...
   * })
   * ```
   * 
   * @param {string} rule_name: name of added rule.
   * @param {function} fn: rule function.
   * @param {dict|nil} options: rule options (optional).
   */
  push(rule_name, fn, options) {
    var opt = options or {}
  
    self.__rules__.append({
      name: rule_name,
      enabled: true,
      fn: fn,
      alt: opt.get('alt', [])
    })
  
    self.__cache__ = nil
  }

  /**
   * Enable rules with given names. If any rule name not found - dies Exception.
   * Errors can be disabled by second param.
   *
   * Returns list of found rule names (if no exception happened).
   *
   * See also [[Ruler.disable]], [[Ruler.enable_only]].
   * 
   * @param {string|list} list: list of rule names to enable.
   * @param {bool} ignore_invalid: set `true` to ignore errors when rule not found.
   * @return list
   */
  enable(list, ignore_invalid) {
    if !is_list(list) list = [ list ]
  
    var result = []
  
    # Search by name and enable
    iters.each(list, @(name) {
      var idx = self.__find__(name)
  
      if idx < 0 {
        if ignore_invalid return
        die Exception('Rules manager: invalid rule name ' + name);
      }
      self.__rules__[idx].enabled = true
      result.append(name)
    })
  
    self.__cache__ = nil
    return result
  }

  /**
   * Enable rules with given names, and disable everything else. If any rule name
   * not found - throw Error. Errors can be disabled by second param.
   *
   * See also [[Ruler.disable]], [[Ruler.enable]].
   * 
   * @param {string|list} list: list of rule names to enable (whitelist).
   * @param {bool} ignore_invalid: set `true` to ignore errors when rule not found.
   */
  enable_only(list, ignore_invalid) {
    if !is_list(list) list = [ list ]
  
    iters.each(self.__rules__, @(rule) { rule.enabled = false })
    self.enable(list, ignore_invalid)
  }

  /**
   * Disable rules with given names. If any rule name not found - throw Error.
   * Errors can be disabled by second param.
   *
   * Returns list of found rule names (if no exception happened).
   *
   * See also [[Ruler.enable]], [[Ruler.enable_only]].
   * 
   * @param {string|list} list: list of rule names to disable.
   * @param {bool} ignore_invalid: set `true` to ignore errors when rule not found.
   * @return list
   */
  disable(list, ignore_invalid) {
    if !is_list(list) list = [ list ]
  
    var result = []
  
    # Search by name and disable
    iters.each(list, @(name) {
      var idx = self.__find__(name)
  
      if idx < 0 {
        if ignore_invalid return
        die Exception('Rules manager: invalid rule name ' + name)
      }
      self.__rules__[idx].enabled = false
      result.append(name)
    })
  
    self.__cache__ = nil
    return result
  }

  /**
   * Return list of active functions (rules) for given chain name. It analyzes
   * rules configuration, compiles caches if not exists and returns result.
   *
   * Default chain name is `''` (empty string). It can't be skipped. That's
   * done intentionally, to keep signature monomorphic for high speed.
   * 
   * @param {string} chain_name
   * @return string
   **/
  get_rules(chain_name) {
    if self.__cache__ == nil {
      self.__compile__()
    }
  
    # Chain can be empty, if rules disabled. But we still have to return list.
    return self.__cache__.get(chain_name, [])
  }
}


