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
 * @copyright Richard Ore, 2025
 */

import _reflect


/**
 * The enum class provides the interface for creating enumerations.
 *
 * @class Enum
 */
class Enum {

  var _keymap = []
  var _value_map = []

  /**
   * The constructor of the Enum class accepts a list of symbolic names or a
   * dictionary of name to unique value mapping and returns a copy of the Enum
   * class that represents the enumeration.
   *
   * @param {list|dict} data
   * @constructor
   */
  Enum(data, unique) {
    var data_is_list = is_list(data)
    if unique == nil unique = true

    assert data_is_list or is_dict(data),
      'enumeration must be a list or dictionary'
    assert is_bool(unique),
      'second argument (unique) must be boolean'
    if !unique {
      assert !data_is_list,
        'unique enumerations can only be initialized with a dictionary'
    }

    var seen_values = []

    for key, value in data {
      var working_key = data_is_list ? value : key
      var working_value = data_is_list ? key : value

      assert is_string(working_key), 'invalid enumeration key'
      assert is_int(working_value), 'invalid enumeration value'
      assert !_reflect.hasprop(self, working_key),
        'duplicate enumeration key "${working_key}"'
      assert !seen_values.contains(working_value),
        'duplicate enumeration value "${working_value}" at "${working_key}"'

      seen_values.append(working_value)
      _reflect.setprop(self, working_key, working_value)

      self._keymap.append(working_key)
      self._value_map.append(working_value)
    }

    # free up memory used by value tracking
    seen_values.clear()
  }

  /**
   * Returns the symbolic keys of an enumeration.
   *
   * @param Enum enum
   * @returns list[string]
   */
  static keys(enum) {
    return _reflect.getprop(enum, '_keymap')
  }

  /**
   * Returns possible numeric values of an enumeration.
   *
   * @param Enum enum
   * @returns list[number]
   */
  static values(enum) {
    return _reflect.getprop(enum, '_value_map')
  }

  /**
   * Returns `true` if the enumeration contains the given symbolic key or
   * `false` if otherwise.
   *
   * @param Enum enum
   * @param string key
   * @returns list[number]
   */
  static has(enum, key) {
    return _reflect.hasprop(enum, key)
  }

  /**
   * Returns the value of an enumeration if it is a valid value for the enumeration
   * or raises Exception if the value is invalid
   *
   * @param Enum enum
   * @param any value
   * @returns any
   * @raises Exception
   */
  static ensure(enum, value) {
    var values = Enum.values()

    if values.contains(value) {
      values.clear()  # free memory immediately
      return value
    }

    values.clear()  # free memory immediately
    raise Exception('unknown key in specified enumeration')
  }
}

/**
 * The default export of the enum module used to create enums.
 */
var enum = Enum
