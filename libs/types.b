# 
# @module type
# 
# provides type validation and conversion capabilities
# 
# this module is wrapper around the builtin functions
# where applicable and does and return the same thing as the builtin
# alternative.
# 
# @copyright 2021, Ore Richard Muyiwa
# 


/**
 * of(value: any)
 *
 * returns the name of the type of value
 * @note method implemented as part of core language features
 */
def of(value) { 
  return typeof(value)
}

/**
 * digit(value: char)
 *
 * returns true if the value is a character and digit,
 * otherwise returns false
 * @return bool
 */
def digit(value) {
  if !is_string(value) or !value.length() == 1
    die Exception('char expected')
  var _ = ord(value)
  return _ >= 48 and _ <= 57
}

/**
 * alpha(value: char)
 *
 * returns true if the value is a character and alphabetic,
 * otherwise returns false
 * @return bool
 */
def alpha(value) {
  if !is_string(value) or !value.length() == 1
    die Exception('char expected')
  var _ = ord(value)
  return (_ >= 65 and _ <= 90) or (_ >= 97 and _ <= 122)
}

/**
 * int(value: any)
 *
 * returns true if the value is an integer or false otherwise
 * @return bool
 */
def int(value) {
  return is_int(value)
}

/**
 * bool(value: any)
 *
 * returns true if the value is a boolean or false otherwise
 * @return bool
 */
def bool() {
  return is_bool(value)
}

/**
 * is_number(value: any)
 *
 * returns true if the value is a number or false otherwise
 * @note this method also returns true for integers
 * @return bool
 */
def number(value) {
  return is_number(value)
}

/**
 * char(value: any)
 *
 * returns true if the value is a single character or false otherwise
 * @return bool
 */
def char(value) {
  return is_string(value) and value.length() == 1
}

/**
 * string(value: any)
 *
 * returns true if the value is a string or false otherwise
 * @return bool
 */
def string(value) {
  return is_string(value)
}

/**
 * bytes(value: any)
 *
 * returns true if the value is a bytes or false otherwise
 * @return bool
 */
def bytes(value) {
  return is_bytes(value)
}

/**
 * list(value: any)
 *
 * returns true if the value is a list or false otherwise
 * @return bool
 */
def list(value) {
  return is_list(value)
}

/**
 * is_dict(value: any)
 *
 * returns true if the value is a dictionary or false otherwise
 * @return bool
 */
def dict(value) {
  return is_dict(value)
}

/**
 * object(value: any)
 *
 * returns true if the value is an object or false otherwise
 * @return bool
 */
def object(value) {
  return is_object(value)
}

/**
 * function(value: any)
 *
 * returns true if the value is a function or false otherwise
 * @return bool
 */
def function(value) {
  return is_function(value)
}

/**
 * class(value: any)
 *
 * returns true if the value is a class or false otherwise
 * @return bool
 */
def is_a_class() {
  return is_class(value)
}

/**
 * file(value: any)
 *
 * returns true if the value is a file or false otherwise
 * @return bool
 */
def file(value) {
  return is_file(value)
}

/**
 * iterable(value: any)
 *
 * returns true if the value is an iterable or false otherwise
 * @return bool
 */
def iterable() {
  return is_iterable(value)
}

/**
 * callable(value: any)
 *
 * returns true if the value is a callable function or class and false otherwise
 * @return bool
 */
def callable() {
  return is_callable(value)
}

/**
 * instance(value: any, type: class)
 *
 * returns true if the value is an instance the given class, false
 * otherwise
 * @return bool
 */
def instance(value, type) {
  if !is_class(type)
    die Exception('class expected')
  return instance_of(value, type)
}


/**
 * @class Convert
 * 
 * handles conversion from one type to another
 */
class Convert {

  /**
   * @constructor Convert
   *  
   * Convert(value: any)
   * set's the value to be converted.
   * @return Convert
   */
  Convert(value) {
    self.value = value
  }

  /**
   * to_int()
   *
   * convert the value into an integer.
   * @note classes may override the return value by declaring a to_int()
   * function.
   * @return bool
   */
  to_int() {
    return to_int(self.value)
  }

  /**
   * to_number()
   *
   * convert the value into a number.
   * @note classes may override the return value by declaring a to_number()
   * function.
   * @return bool
   */
  to_number() {
    return to_number(self.value)
  }

  /**
   * to_string()
   *
   * convert the value into a string.
   * @note classes may override the return value by declaring a to_string()
   * function.
   * @return bool
   */
  to_string() {
    return to_string(self.value)
  }

  /**
   * to_bool()
   *
   * converts the value into a boolean.
   * @note classes may override the return value by declaring a to_bool()
   * function.
   * @return bool
   */
  to_bool() {
    return to_bool(self.value)
  }

  /**
   * to_list()
   *
   * convert the value into a list.
   * @note classes may override the return value by declaring a to_list()
   * function.
   * @return bool
   */
  to_list() {
    return to_list(self.value)
  }

  /**
   * to_dict()
   *
   * convert the value value into a dictionary.
   * @note classes may override the return value by declaring a to_dict()
   * function.
   * @return bool
   */
  to_dict() {
    return to_dict(self.value)
  }
}

