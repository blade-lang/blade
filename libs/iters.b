#
# @module iters
# 
# Provides functions for simplifying the usage of iterables.
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
# 


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
    die Exception('arg1 must be an iterable')
  if !is_function(callback)
    die Exception('arg2 must be a function')

  for index, item in object {
    callback(item, index)
  }
}

/**
 * reduce(list: list, callback: function [, initial: any])
 * 
 * Executes a user-supplied "reducer" callback function on each element 
 * of the list, in order, passing in the return value from the calculation 
 * on the preceding element. 
 * The final result of running the reducer across all elements of the list 
 * is a single value.
 * 
 * The first time that the callback is run there is no "return value of the 
 * previous calculation". If supplied, an initial value may be used in its 
 * place. Otherwise the list element at index 0 is used as the initial value 
 * and iteration starts from the next element (index 1 instead of index 0).
 */
def reduce(list, callback, initial) {
  if !is_list(list)
    die Exception('arg1 must be a list')
  if !is_function(callback)
    die Exception('arg2 must be a function')

  if initial == nil and !list.is_empty()
    initial = list[0]

  for item in list {
    initial = callback(initial, item)
  }

  return initial
}

/**
 * map(list: list, callback: function)
 * 
 * Creates a new list populated with the results of calling the provided 
 * callback on every element in the list.
 */
def map(list, callback) {
  if !is_list(list)
    die Exception('arg1 must be a list')
  if !is_function(callback)
    die Exception('arg2 must be a function')

  var result = []

  for item in list 
    result.append(callback(item))

  return result
}

/**
 * some(list: list, callback: function)
 * 
 * Tests whether at least one element in the list passes the test 
 * implemented by the provided function. It returns true if, in the list, 
 * it finds an element for which the provided function returns true; 
 * otherwise it returns false.
 */
def some(list, callback) {
  for item in list {
    if callback(item) return true
  }
  return false
}

/**
 * every(list: list, callback: function)
 * 
 * Tests whether all elements in the list passes the test implemented by 
 * the provided function. It returns false if, in the list, it finds an 
 * element for which the provided function returns false.
 */
def every(list, callback) {
  for item in list {
    if !callback(item) return false
  }
  return true
}

/**
 * filter(list: list, callback: function)
 * 
 * Creates a new list with all elements that pass the test implemented by 
 * the provided function.
 */
def filter(list, callback) {
  var result = []

  for item in list {
    if callback(item) result.append(item)
  }

  return result
}
