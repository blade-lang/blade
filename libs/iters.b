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

