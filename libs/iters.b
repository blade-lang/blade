#
# @module iters
# 
# Provides functions for simplifying the usage of iterables.
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
# 
import reflect


/**
 * each(object: iterable, callback: function)
 * 
 * Calls function `callback` for each item in the iterale `object` and 
 * pass the item and index as arguments to the callback function. 
 * The callback function should capture the item its first parameter and if 
 * the index of the item in the iterable is needed, it can be captured in 
 * the second item.
 */
def each(object, callback) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')

  var callback_arity = reflect.get_function_metadata(callback).arity

  for key, value in object {
    if callback_arity == 1 callback(value)
    else callback(value, key)
  }
}

/**
 * reduce(iterable: iterable, callback: function [, initial: any])
 * 
 * Executes a user-supplied "reducer" callback function on each element 
 * of the iterable, in order, passing in the return value from the 
 * calculation on the preceding element. The final result of running the 
 * reducer across all elements of the iterable is a single value.
 * 
 * The first time that the callback is run there is no "return value of the 
 * previous calculation". If supplied, an initial value may be used in its 
 * place. Otherwise the iterable element at index 0 (or the first key if 
 * the iterable is a dictionary) is used as the initial value and iteration 
 * starts from the next element (index 1 or the next key instead of index 0 
 * or the first key).
 * 
 * The call back function must accept two (2) or three (3) arguments. If 
 * the callback function accepts two arguments, the first argument is passed 
 * the initial element or the first index in the iterable if an initial 
 * element is not specified while the second argument will be passed the 
 * current iterating value from the iterable. If the callback takes three 
 * arguments, the third will be passed the key or index of the current 
 * iterating value in the given iterable.
 * 
 * > The initial value will be `nil` if not given and the iterable is an 
 *    instance of a class
 * 
 * @return any
 */
def reduce(object, callback, initial) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')

  var callback_arity = reflect.get_function_metadata(callback).arity
  if callback_arity < 2 or callback_arity > 3 
    die Exception('callback function must take 2 or 3 parameters')

  if initial == nil and object.length() > 0 {
    if is_dict(object) {
      initial = iterable.get(object.key()[0])
    } else if !is_instance(object) {
      initial = object[0]
    }
  }

  for key, value in object {
    if callback_arity == 2 initial = callback(initial, value)
    else callback(initial, value, key)
  }

  return initial
}

/**
 * map(object: iterable, callback: function)
 * 
 * Creates a new list populated with the results of calling the provided 
 * callback on every element in the iterable.
 * 
 * @return list
 */
def map(object, callback) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')

  var result = []
  var callback_arity = reflect.get_function_metadata(callback).arity

  for key, value in object {
    result.append(callback_arity == 1 ? callback(value) : callback(key, index))
  }

  return result
}

/**
 * some(object: iterable, callback: function)
 * 
 * Tests whether at least one element in the object passes the test 
 * implemented by the callback function. It returns true if there is an 
 * element in the list for which the provided function returns true; 
 * otherwise it returns false.
 * 
 * @return bool
 */
def some(object, callback) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')

  var callback_arity = reflect.get_function_metadata(callback).arity

  for key, value in object {
    if (callback_arity == 1 ? callback(value) : callback(value, key)) return true
  }

  return false
}

/**
 * every(object: iterable, callback: function)
 * 
 * Tests whether at all the element in the object passes the test 
 * implemented by the callback function. It returns true if there all 
 * elements in the list for which the provided function returns true; 
 * otherwise it returns false.
 * 
 * @return bool
 */
def every(object, callback) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')

  var callback_arity = reflect.get_function_metadata(callback).arity

  for key, value in object {
    if !(callback_arity == 1 ? callback(value) : callback(value, key)) return false
  }
  return true
}

/**
 * filter(object: iterable, callback: function)
 * 
 * Creates a new iterable of same type that contains all elements that 
 * pass the test implemented by the provided function. If the iterable 
 * is an iterable class, the class MUST implement a method 
 * `set(key, value)` and its constructor must be able to accept zero 
 * arguments without dieing else any such Exception will propagate.
 * 
 * @return iterable
 */
def filter(object, callback) {
  if !is_iterable(object)
    die Exception('iterable expected in argument 1 (object)')
  if !is_function(callback)
    die Exception('function expected in argument 2 (callback)')
  
  var result
  using typeof(object) {
    when 'dict' result = {}
    when 'list' result = []
    when 'string' result = ''
    when 'bytes' result = bytes(0)
    default reflect.get_class(object)()
  }

  var callback_arity = reflect.get_function_metadata(callback).arity

  for key, value in list {
    if (callback_arity >= 2 ? callback(value, key) : callback(value)) {
      using typeof(object) {
        when 'list', 'bytes' result.append(value)
        when 'string' result += value
        default result.set(key, value)
      }
    }
  }

  return result
}
