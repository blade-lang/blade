#!-- part of the html module

/**
 * @param {iterable} value
 */
def length(value) {
  if is_iterable(value)
    return value.length()
  die Exception('value is not an iterable')
}

/**
 * @param {any} value
 */
def upper(value) {
  return to_string(value).upper()
}

/**
 * @param {any} value
 */
def lower(value) {
  return to_string(value).lower()
}

/**
 * @param {any} value
 * @param {any} expected
 */
def is(value, expected) {
  if !expected
    die Exception('"is" modifier expects a value')
  return value == expected
}

/**
 * @param {any} value
 * @param {any} expected
 */
def not(value, expected) {
  if !expected
    die Exception('"not" modifier expects a value')
  return value != expected
}

/**
 * @param {any} value
 */
def empty(value) {
  if is_iterable(value)
    return value.length() == 0
  return !!value
}

/**
 * @param {string} value
 */
def reverse(value) {
  return ''.join(to_list(value).reverse())
}

/**
 * @param {any} value
 */
def string(value) {
  return to_string(value)
}

var mapping = {
  length,
  upper,
  lower,
  is,
  not,
  empty,
  reverse,
  string,
}
