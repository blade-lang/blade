/**
 * Type
 *
 * Bird's type validation and conversion library
 *
 * This class is an object oriented wrapper to the builtin functions
 * where applicable and does and return the same thing as the builtin
 * alternative.
 * @copyright 2021, Ore Richard Muyiwa
 */
class Type {

  # Constructor
  # Type()
  Type(value) {
    self.value = value
  }

  /**
   * type(value: any)
   * returns the name of the type of value
   *
   * @note method implemented as part of core language features
   */
  static of(value) { 
    return typeof(value)
  }

  ### VALIDATION FUNCTIONS

  /**
   * is_int()
   * returns true if the value is an integer or false otherwise
   */
  is_int() {
    return is_int(self.value)
  }

  /**
   * is_bool()
   * returns true if the value is a boolean or false otherwise
   */
  is_bool() {
    return is_bool(self.value)
  }

  /**
   * is_number()
   * returns true if the value is a number or false otherwise
   *
   * @note this method also returns true for integers
   */
  is_number() {
    return is_number(self.value)
  }

  /**
   * is_char()
   * returns true if the value is a single character or false otherwise
   */
  is_char() {
    return is_string(self.value) and self.value.length() == 1
  }

  /**
   * is_string()
   * returns true if the value is a string or false otherwise
   */
  is_string() {
    return is_string(self.value)
  }

  /**
   * is_list()
   * returns true if the value is a list or false otherwise
   */
  is_list() {
    return is_list(self.value)
  }

  /**
   * is_dict()
   * returns true if the value is a dictionary or false otherwise
   */
  is_dict() {
    return is_dict(self.value)
  }

  /**
   * is_object()
   * returns true if the value is an object or false otherwise
   */
  is_object() {
    return is_object(self.value)
  }

  /**
   * is_function()
   * returns true if the value is a function or false otherwise
   */
  is_function() {
    return is_function(self.value)
  }

  /**
   * is_class()
   * returns true if the value is a class or false otherwise
   */
  is_class() {
    return is_class(self.value)
  }

  /**
   * is_file()
   * returns true if the value is a file or false otherwise
   */
  is_file() {
    return is_file(self.value)
  }

  /**
   * is_iterable()
   * returns true if the value is an iterable or false otherwise
   */
  is_iterable() {
    return is_iterable(self.value)
  }

  /**
   * is_callable()
   * returns true if the value is a callable function or class and false otherwise
   */
  is_callable() {
    return is_callable(self.value)
  }

  /**
   * is_instance(type: class)
   * returns true if the value is an instance the given class, false
   * otherwise
   */
  is_instance(type) {
    if !Type.is_class(type)
      die Exception('class expected')
    return is_instance(self.value, type)
  }

  /**
   * is_digit()
   * returns true if the value is a character and digit,
   * otherwise returns false
   */
  is_digit() {
    if !is_string(self.value) or !self.value.length() == 1
      die Exception('char expected')
    var _ = ord(self.value)
    return _ >= 48 and _ <= 56
  }

  /**
   * is_alpha()
   * returns true if the value is a character and alphabetic,
   * otherwise returns false
   */
  is_alpha() {
    if !is_string(self.value) or !self.value.length() == 1
      die Exception('char expected')
    var _ = ord(self.value)
    return (_ >= 65 and _ <= 90) or (_ >= 97 and _ <= 121)
  }

  ### CONVERSION FUNTIONS

  /**
   * to_int()
   * convert the value into an integer.
   *
   * classes may override the return value by declaring a to_int()
   * function.
   */
  to_int() {
    return to_int(self.value)
  }

  /**
   * to_number()
   * convert the value into a number.
   *
   * classes may override the return value by declaring a to_number()
   * function.
   */
  to_number() {
    return to_number(self.value)
  }

  /**
   * to_string()
   * convert the value into a string.
   *
   * classes may override the return value by declaring a to_string()
   * function.
   */
  to_string() {
    return to_string(self.value)
  }

  /**
   * to_bool()
   * converts the value into a boolean.
   *
   * classes may override the return value by declaring a to_bool()
   * function.
   */
  to_bool() {
    return to_bool(self.value)
  }

  /**
   * to_list()
   * convert the value into a list.
   *
   * classes may override the return value by declaring a to_list()
   * function.
   */
  to_list() {
    return to_list(self.value)
  }

  /**
   * to_dict()
   * convert the value value into a dictionary.
   *
   * classes may override the return value by declaring a to_dict()
   * function.
   */
  to_dict() {
    return to_dict(self.value)
  }
}

