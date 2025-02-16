/**
 * @module enum
 *
 * This module provides support for enumerations in Blade.
 *
 * An enumeration in Blade is a set of unique values bound to symbolic
 * names via an alias.
 *
 * Enums are single state instances similar to a dictionary but having
 * automatically or distinctly assigned values.
 * 
 * You can create an enumeration by calling the `enum()` function and 
 * initializing it with either a list of constants (as string) or a 
 * dictionary. When an enumeration is initialized with a list, individual 
 * items in the list are automatically assigned an ordinal value 
 * starting from zero (0). 
 * 
 * For example:
 * 
 * ```blade-repl
 * %> import enum
 * %> 
 * %> var Gender = enum(['Male', 'Female'])
 * %> to_string(Gender)
 * '<Enum Male=0 Female=1>'
 * ```
 * 
 * If an enumeration is initialized with a dictionary, the keys are 
 * automatically bound to the values as defined in the dictionary. This can 
 * be useful if your enumeration requires values that are not in order or 
 * require specific values. When an enumeration is initialized with a 
 * dictionary, the values of the enumeration constants can be either a 
 * number or a string. 
 * 
 * For example:
 * 
 * ```blade-repl
 * %> import enum
 * %> 
 * %> var Color = enum({
 * ..   Red: 'r',
 * ..   Green: 'g',
 * ..   Blue: 'b',
 * .. })
 * %> echo to_string(Color)
 * '<Enum Red=r Green=g Blue=b>'
 * ```
 * 
 * Enums can never contain duplicate keys and do not _normally_ allow 
 * duplicate values as they are unique by default and an attempt to 
 * duplicate a key or value will raise an Exception. 
 * 
 * For example:
 * 
 * ```blade-repl
 * %> import enum
 * %> 
 * %> var Speed = enum({
 * ..   Slow: 1,
 * ..   Sluggish: 1,
 * ..   Fast: 2,
 * .. })
 * Illegal State: duplicate enumeration value "1" at "Sluggish"
 *   StackTrace:
 *     <repl>:5 -> @.script()
 * ```
 * 
 * To allow duplicate values in an enumeration if desired, you can pass 
 * a second argument _false_ after the initialization data to disable 
 * uniqueness like below.
 * 
 * ```blade-repl
 * %> import enum
 * %> 
 * %> var Speed = enum({
 * ..   Slow: 1,
 * ..   Sluggish: 1,
 * ..   Fast: 2,
 * .. }, false)
 * %> echo to_string(Speed)
 * '<Enum Slow=1 Sluggish=1 Fast=2>'
 * ```
 * 
 * > **NOTE:**
 * >
 * > Duplicate values are only allowed in enumerations initialized 
 * > with a dictionary.
 * 
 * The value of an enumeration key can be retrieved from the enumeration 
 * object itself like in the example below.
 * 
 * ```blade-repl
 * %> import enum
 * %> 
 * %> # list initialization
 * %> var Gender = enum(['Male', 'Female'])
 * %> 
 * %> Gender.Male
 * 0
 * %> Gender.Female
 * 1
 * ```
 *
 * @copyright Richard Ore, 2025
 */

import _reflect


/**
 * The enum class provides the interface for creating enumerations.
 *
 * @printable
 */
class Enum {

  /**
   * The constructor of the Enum class accepts a list of symbolic names or a
   * dictionary of name to unique value mapping and returns a copy of the Enum
   * class that represents the enumeration.
   *
   * @param {list|dict} data
   * @param bool? unique = true
   * @constructor
   */
  Enum(data, unique) {
    var data_is_list = is_list(data)
    if unique == nil unique = true

    assert data_is_list or is_dict(data),
      'enumeration must be a list or dictionary'
    assert is_bool(unique),
      'second argument (unique) must be boolean'
    assert to_bool(data), 'cannot create empty enumeration'

    if !unique {
      assert !data_is_list,
        'unique enumerations can only be initialized with a dictionary'
    }

    var seen_values = []

    for key, value in data {
      var working_key = data_is_list ? value : key
      var working_value = data_is_list ? key : value

      assert is_string(working_key), 'invalid enumeration key'
      assert is_int(working_value) or is_string(working_value), 'invalid enumeration value'
      assert !_reflect.hasprop(self, working_key),
        'duplicate enumeration key "${working_key}"'
      if unique {
        assert !seen_values.contains(working_value),
          'duplicate enumeration value "${working_value}" at "${working_key}"'
      }

      seen_values.append(working_value)
      _reflect.setprop(self, working_key, working_value)
    }

    # free up memory used by value tracking
    seen_values.clear()
  }

  /**
   * Returns the symbolic keys of an enumeration.
   * 
   * For example:
   * 
   * ```blade-repl
   * %> var Color = enum({
   * ..   Red: 'r',
   * ..   Green: 'g',
   * ..   Blue: 'b',
   * .. })
   * %> 
   * %> enum.keys(Color)
   * [Red, Green, Blue]
   * ```
   *
   * @param Enum enum
   * @returns list[string]
   */
  static keys(enum) {
    return _reflect.getprops(enum)
  }

  /**
   * Returns possible numeric values of an enumeration.
   * 
   * For example:
   * 
   * ```blade-repl
   * %> var Gender = enum(['Male', 'Female'])
   * %> 
   * %> enum.values(Gender)
   * [0, 1]
   * ```
   *
   * @param Enum enum
   * @returns list[number|string]
   */
  static values(enum) {
    return Enum.keys(enum).map(@(key) {
      return _reflect.getprop(enum, key)
    })
  }

  /**
   * Returns the enumeration as a key/value dictionary.
   * 
   * For example:
   * 
   * ```blade-repl
   * %> var Gender = enum(['Male', 'Female'])
   * %> 
   * %> enum.to_dict(Gender)
   * {Male: 0, Female: 1}
   * ```
   *
   * @param Enum enum
   * @returns dict
   */
  static to_dict(enum) {
    return Enum.keys(enum).reduce(@(prev, key) {
      prev.set(key, _reflect.getprop(enum, key))
      return prev
    }, {})
  }

  /**
   * Returns the enumeration as a value/key dictionary.
   * 
   * For example:
   * 
   * ```blade-repl
   * %> var Speed = enum({
   * ..   Slow: 1,
   * ..   Sluggish: 1,
   * ..   Fast: 2,
   * .. }, false)
   * %> 
   * %> enum.to_value_dict(Speed)
   * {1: Sluggish, 2: Fast}
   * ```
   * 
   * > **NOTE:**
   * > 
   * > It is important to remember that dictionaries cannot contain
   * > duplicates themselves so all enumeration keys that share the
   * > same value will be represented as one. This is in fact not an 
   * > error since the values are originally the same like in the 
   * > example above where `Speed.Slow` and `Speed.Sluggish` are 
   * > actually the same.
   *
   * @param Enum enum
   * @returns dict
   */
  static to_value_dict(enum) {
    return Enum.keys(enum).reduce(@(prev, key) {
      prev.set(_reflect.getprop(enum, key), key)
      return prev
    }, {})
  }

  /**
   * Returns `true` if the enumeration contains the given symbolic key or
   * `false` if otherwise.
   * 
   * For example:
   * 
   * ```blade-repl
   * %> var Holiday = enum([
   * ..   'Christmas',
   * ..   'Easter',
   * ..   'NewYear'
   * .. ])
   * %> 
   * %> enum.has(Holiday, 'NineEleven')
   * false
   * %> enum.has(Holiday, 'Easter')
   * true
   * ```
   *
   * @param Enum enum
   * @param string key
   * @returns bool
   */
  static has(enum, key) {
    return _reflect.hasprop(enum, key)
  }

  /**
   * Returns the value of an enumeration if it is a valid value for the enumeration
   * 
   * ```blade-repl
   * %> var Gender = enum(['Male', 'Female'])
   * %> %> enum.ensure(Gender, 0)
   * 0
   * ```
   * 
   * or raises Exception if the value is invalid.
   * 
   * ```blade-repl
   * %> var Gender = enum(['Male', 'Female'])
   * %> enum.ensure(Gender, 2)
   * Unhandled Exception: unknown key/value in specified enumeration
   *   StackTrace:
   *     <repl>:1 -> @.script()
   * ```
   *
   * @param Enum enum
   * @param any value
   * @returns any
   * @raises Exception
   */
  static ensure(enum, value) {
    var values = Enum.values(enum)

    if values.contains(value) {
      values.clear()  # free memory immediately
      return value
    }

    values.clear()  # free memory immediately
    raise Exception('unknown key/value in specified enumeration')
  }

  @to_string() {
    var str = '<Enum '
    for k, v in Enum.to_dict(self) {
      str += '${k}=${v} '
    }
    return str.rtrim() + '>'
  }
}

/**
 * The default export of the enum module used to create enums.
 * 
 * See [[enum.Enum]]
 */
def enum(data, unique) {
  return Enum(data, unique)
}

/**
 * Exported Enum.has static function for module access.
 * 
 * See [[enum.Enum.has]]
 */
def has(enum, key) {
  return Enum.has(enum, key)
}

/**
 * Exported Enum.keys static function for module access.
 * 
 * See [[enum.Enum.keys]]
 */
def keys(enum) {
  return Enum.keys(enum)
}

/**
 * Exported Enum.values static function for module access.
 * 
 * See [[enum.Enum.values]]
 */
def values(enum) {
  return Enum.values(enum)
}

/**
 * Exported Enum.ensure static function for module access.
 * 
 * See [[enum.Enum.ensure]]
 */
def ensure(enum, value) {
  return Enum.ensure(enum, value)
}

/**
 * Exported Enum.to_dict static function for module access.
 * 
 * See [[enum.Enum.to_dict]]
 */
def to_dict(enum) {
  return Enum.to_dict(enum)
}

/**
 * Exported Enum.to_value_dict static function for module access.
 * 
 * See [[enum.Enum.to_value_dict]]
 */
def to_value_dict(enum) {
  return Enum.to_value_dict(enum)
}
