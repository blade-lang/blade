#!-- part of the html module

import url
import json as js

/**
 * Template function to return the length of an iterable.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|length }}
 * ```
 * 
 * If _value_ is **Example**, output will be **7**.
 */
def length(value) {
  if is_iterable(value)
    return value.length()
  die Exception('value is not an iterable')
}

/**
 * Template function to convert a string or an object's string representation 
 * to upper case variant.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|upper }}
 * ```
 * 
 * If _value_ is **Example text**, output will be **EXAMPLE TEXT**.
 */
def upper(value) {
  return to_string(value).upper()
}

/**
 * Template function to convert a string or an object's string representation 
 * to lower case variant.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|lower }}
 * ```
 * 
 * If _value_ is **I'm LOVING this**, output will be **i'm loving this**.
 */
def lower(value) {
  return to_string(value).lower()
}

/**
 * Template function to check if object _value_ is same as the _expected_.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|is='Jane' }}
 * ```
 * 
 * If value was **Jane**, it will return `true`.
 * 
 * You can also pass another variable name, a number or one of `true`, 
 * `false`, and `nil` directly (without quotes).
 * 
 * @param any expected
 */
def is(value, expected) {
  if !expected
    die Exception('"is" modifier expects a value')
  return value == expected
}

/**
 * Template function to check if object _value_ is NOT the same as _expected_.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|not=false }}
 * ```
 * 
 * If value was __true__, it will return `false`.
 * 
 * It accepts the same set of parameters accepted by the `is` template modifier.
 * 
 * @param any expected
 */
def not(value, expected) {
  if !expected
    die Exception('"not" modifier expects a value')
  return value != expected
}

/**
 * Template function to check if an iterable is empty.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|empty }}
 * ```
 * 
 * If value is an _empty string_, it will return `true`.
 */
def empty(value) {
  if is_iterable(value)
    return value.length() == 0
  return !!value
}

/**
 * Template function to reverse a string or the string representation of an object.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|reverse }}
 * ```
 * 
 * If value is **banana**, output will be **ananab**.
 */
def reverse(value) {
  return ''.join(to_list(value).reverse())
}

/**
 * Template function to convert an object of any type to a string.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|string }}
 * ```
 * 
 * If value was a list `[1,2,3]`, output will be the string **[1, 2, 3]**.
 */
def string(value) {
  return to_string(value)
}

/**
 * Template function to trim a string.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|trim }}
 * ```
 * 
 * If value is `   Jane   `, output will be `Jane`.
 */
def trim(value) {
  if !is_string(value)
    die Exception('value is not a string')
  return value.trim()
}

/**
 * Template function to convert a string to a title case.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|title }}
 * ```
 * 
 * If value is **jane IS a fine girl**, output will be **Jane Is A Fine Girl**.
 */
def title(value) {
  if !is_string(value)
    die Exception('value is not a string')

  return ' '.join(value.split(' ').map(@(val) {
    val = val.lower()

    return !val ? val : (val[0].upper() + val[1,])
  }))
}

/**
 * Template function to return a default string value if the value 
 * passed resolves to a Blade false expression. For example, when a 
 * string is empty or nil.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|alt=30 }}
 * ```
 * 
 * If value is **-1**, out put will be **30**.
 * 
 * @param string alternative
 */
def alt(value, alternative) {
  return value ? value : alternative
}

/**
 * Template function to return the first item in an iterable.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|first }}
 * ```
 * 
 * If value is a list `['mango', 'apple', 'oranges']`, output will be **mango**.
 */
def first(value) {
  if !is_iterable(value)
    die Exception('value is not an iterable')
  return value ? value[0] : ''
}

/**
 * Template function to return the last item in an iterable.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|last }}
 * ```
 * 
 * If value is a list `['mango', 'apple', 'oranges']`, output will be **oranges**.
 */
def last(value) {
  if !is_iterable(value)
    die Exception('value is not an iterable')
  return value ? value[value.length() - 1] : ''
}

/**
 * Template function to replace newlines with HTML line breaks.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|line_breaks }}
 * ```
 * 
 * If value is `Hello\nWorld`, output will be `Hello<br/>World`.
 */
def line_breaks(value) {
  if !is_string(value)
    die Exception('value is not a string')
  return value.replace('\n', '<br/>')
}

/**
 * Template function to left pad a string.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|lpad=10 }}
 * ```
 * 
 * If value is `Jane`, output will be `      Jane`.
 * 
 * @param number count.
 */
def lpad(value, count) {
  if !is_string(value)
    die Exception('value is not a string')
  if !is_number(count) or !(is_string(count) and count.match('/^\d+(\.\d+)?$/'))
    die Exception('count is not a number')

  return value.lpad(to_number(count))
}

/**
 * Template function to right pad a string.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|rpad=10 }}
 * ```
 * 
 * If value is `Jane`, output will be `Jane      `.
 * 
 * @param number count.
 */
def rpad(value, count) {
  if !is_string(value)
    die Exception('value is not a string')
  if !is_string(count) or !count.match('/^\d+(\.\d+)?$/')
    die Exception('count is not a number')

  return value.rpad(to_number(count))
}

/**
 * Template function to join an iterable using a string glue.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|join='-' }}
 * ```
 * 
 * If value is a list `['a', 'b', 'c']`, output will be **a-b-c**.
 * 
 * @param string glue
 */
def join(value, glue) {
  if !is_iterable(value)
    die Exception('value is not an iterable')
  if !is_string(glue)
    die Exception('glue is not a string')

  return glue.join(to_list(value))
}

/**
 * Template function to return the url encoded value of a string.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|url_encode }}
 * ```
 * 
 * If value is **https://www.example.org/foo?a=b&c=d**, output will be **https://www.example.org/foo%3Fa%3Db&c%3Dd**.
 */
def url_encode(value) {
  if !is_string(value)
    die Exception('value is not a string')
  return url.encode(value)
}

/**
 * Template string to return the JSON encoded string for a value.
 * 
 * Example:
 * 
 * ```wire
 * {{ value|json }}
 * ```
 * 
 * If value is a dictionary `{name: 'Xavier'}`, output will be **{"name":"Xavier"}**.
 */
def json(value) {
  return js.encode(value)
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
  trim,
  title,
  alt,
  first,
  last,
  line_breaks,
  join,
  url_encode,
  json,
}
