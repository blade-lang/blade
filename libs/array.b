#
# @module array
# 
# This moddule provides multiple classes for working with arrays of twos-complement 
# integers in the platform byte order. The classes provided in this module complement
# the _bytes()_ object and allow higher other binary data manipulation.
# 
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
#

import _array

/**
 * Maximum value that "should" exist in a list passed to Int16Array.
 */
var INT16_MAX = 32767

/**
 * Maximum value that "should" exist in a list passed to UInt16Array.
 */
var UINT16_MAX = 65535

/**
 * Maximum value that "should" exist in a list passed to Int32Array.
 */
var INT32_MAX = 2147483647

/**
 * Maximum value that "should" exist in a list passed to UInt32Array.
 */
var UINT32_MAX = 4294967295

/**
 * Maximum value that "should" exist in a list passed to Int64Array.
 */
var INT64_MAX = 9223372036854775807

/**
 * Maximum value that "should" exist in a list passed to UInt64Array.
 */
var UINT64_MAX = 18446744073709551615

/**
 * Minimum value that "should" exist in a list passed to Int16Array.
 */
var INT16_MIN = -INT16_MAX - 1

/**
 * Minimum value that "should" exist in a list passed to Int32Array.
 */
var INT32_MIN = -INT32_MAX - 1

/**
 * Minimum value that "should" exist in a list passed to Int64Array.
 */
var INT64_MIN = -INT64_MAX - 1


/**
 * class Int16Array represents an array of twos-complement 16-bit signed 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class Int16Array {

  /**
   * Int16Array(n: number | list)
   * 
   * - If n is a number, it creates a new Int16Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int16Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  Int16Array(n) {
    self._ptr = _array.Int16Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT16_MIN or value > INT16_MAX
      die Exception('value out of int16 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.int16_get(self._ptr, index)
  }

  /**
   * extend(array: Int16Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, Int16Array)
      die Exception('instance of Int16Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int16_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new Int16Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return Int16Array
   */
  clone() {
    return Int16Array(_array.int16_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.int16_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.int16_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.int16_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw int16 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.int16___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class Int32Array represents an array of twos-complement 32-bit signed 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class Int32Array {

  /**
   * Int32Array(n: number | list)
   * 
   * - If n is a number, it creates a new Int32Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int32Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  Int32Array(n) {
    self._ptr = _array.Int32Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT32_MIN or value > INT32_MAX
      die Exception('value out of int32 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.int32_get(self._ptr, index)
  }

  /**
   * extend(array: Int32Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, Int32Array)
      die Exception('instance of Int32Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int32_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new Int32Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return Int32Array
   */
  clone() {
    return Int32Array(_array.int32_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.int32_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.int32_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.int32_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw int32 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.int32___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class Int64Array represents an array of twos-complement 64-bit signed 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class Int64Array {

  /**
   * Int64Array(n: number | list)
   * 
   * - If n is a number, it creates a new Int64Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int64Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  Int64Array(n) {
    self._ptr = _array.Int64Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT64_MIN or value > INT64_MAX
      die Exception('value out of int64 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.int64_get(self._ptr, index)
  }

  /**
   * extend(array: Int64Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, Int64Array)
      die Exception('instance of Int64Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int64_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new Int64Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return Int64Array
   */
  clone() {
    return Int64Array(_array.int64_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.int64_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.int64_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.int64_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw int64 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.int64___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class UInt16Array represents an array of twos-complement 16-bit unsigned 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class UInt16Array {

  /**
   * UInt16Array(n: number | list)
   * 
   * - If n is a number, it creates a new UInt16Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt16Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  UInt16Array(n) {
    self._ptr = _array.UInt16Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT16_MIN or value > INT16_MAX
      die Exception('value out of uint16 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.uint16_get(self._ptr, index)
  }

  /**
   * extend(array: UInt16Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, UInt16Array)
      die Exception('instance of UInt16Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint16_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new UInt16Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return UInt16Array
   */
  clone() {
    return UInt16Array(_array.uint16_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.uint16_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.uint16_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.uint16_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw uint16 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.uint16___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class UInt32Array represents an array of twos-complement 32-bit unsigned 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class UInt32Array {

  /**
   * UInt32Array(n: number | list)
   * 
   * - If n is a number, it creates a new UInt32Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt32Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  UInt32Array(n) {
    self._ptr = _array.UInt32Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT32_MIN or value > INT32_MAX
      die Exception('value out of uint32 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.uint32_get(self._ptr, index)
  }

  /**
   * extend(array: UInt32Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, UInt32Array)
      die Exception('instance of UInt32Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint32_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new UInt32Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return UInt32Array
   */
  clone() {
    return UInt32Array(_array.uint32_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.uint32_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.uint32_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.uint32_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw uint32 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.uint32___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class UInt64Array represents an array of twos-complement 64-bit unsigned 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class UInt64Array {

  /**
   * UInt64Array(n: number | list)
   * 
   * - If n is a number, it creates a new UInt64Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt64Array with its elements set to 
   * the values in the list.
   * @constructor
   */
  UInt64Array(n) {
    self._ptr = _array.UInt64Array(n)
  }

  /**
   * length()
   * 
   * Returns the number of items in the array. 
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * bytes_length()
   * 
   * Returns the length of the array if it were to be converted to bytes.
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * first()
   * 
   * Returns the first item in the array or nil if the array is empty.
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * last()
   * 
   * Returns the last item in the array or nil if the array is empty.
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * append(value: int)
   * 
   * Adds the given _value_ to the end of the array.
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      die Exception('integer expected')
    if value < INT64_MIN or value > INT64_MAX
      die Exception('value out of uint64 range')

    _array.append(self._ptr, value)
  }

  /**
   * get(index: number)
   * 
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * @return number
   */
  get(index) {
    if !is_number(index)
      die Exception('Arrays are numerically indexed')
    
    return _array.uint64_get(self._ptr, index)
  }

  /**
   * extend(array: UInt64Array)
   * 
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   */
  extend(array) {
    if !instance_of(array, UInt64Array)
      die Exception('instance of UInt64Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * reverse()
   * 
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint64_reverse(self._ptr)
  }

  /**
   * clone()
   * 
   * Returns a new UInt64Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * @return UInt64Array
   */
  clone() {
    return UInt64Array(_array.uint64_clone(self._ptr))
  }

  /**
   * pop()
   * 
   * Removes the last element in the array and returns the value of that item.
   * @return number
   */
  pop() {
    return _array.uint64_pop(self._ptr)
  }

  /**
   * to_bytes()
   * 
   * Returns the array as a bytes object
   * @return bytes
   */
  to_bytes() {
    return _array.uint64_to_bytes(self._ptr)
  }

  /**
   * to_list()
   * 
   * Returns the elements of the array as a list of numbers
   * @return list
   */
  to_list() {
    return _array.uint64_to_list(self._ptr)
  }

  /**
   * to_string()
   * 
   * Returns a string representation of the array
   * @return string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * Returns the raw uint64 array pointer.
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @iter(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.uint64___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      die Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}
