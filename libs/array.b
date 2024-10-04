/**
 * @module array
 * 
 * This moddule provides multiple classes for working with arrays of twos-complement 
 * integers in the platform byte order. The classes provided in this module complement
 * the _bytes()_ object and allow higher other binary data manipulation.
 * 
 * @copyright 2022, Ore Richard Muyiwa and Blade contributors
 */

import _array

/**
 * Maximum value that "should" exist in a list passed to Int16Array.
 * @type number
 */
var INT16_MAX = 32767

/**
 * Maximum value that "should" exist in a list passed to UInt16Array.
 * @type number
 */
var UINT16_MAX = 65535

/**
 * Maximum value that "should" exist in a list passed to Int32Array.
 * @type number
 */
var INT32_MAX = 2147483647

/**
 * Maximum value that "should" exist in a list passed to UInt32Array.
 * @type number
 */
var UINT32_MAX = 4294967295

/**
 * Maximum value that "should" exist in a list passed to Int64Array.
 * @type number
 */
var INT64_MAX = 9223372036854775807

/**
 * Maximum value that "should" exist in a list passed to UInt64Array.
 * @type number
 */
var UINT64_MAX = 18446744073709551615

/**
 * Maximum value that "should" exist in a list passed to FloatArray.
 * @type number
 */
var FLOAT_MAX = 3.402823466E+38

/**
 * Minimum value that "should" exist in a list passed to Int16Array.
 * @type number
 */
var INT16_MIN = -INT16_MAX - 1

/**
 * Minimum value that "should" exist in a list passed to Int32Array.
 * @type number
 */
var INT32_MIN = -INT32_MAX - 1

/**
 * Minimum value that "should" exist in a list passed to Int64Array.
 * @type number
 */
var INT64_MIN = -INT64_MAX - 1

/**
 * Minimum value that "should" exist in a list passed to FloatArray.
 * @type number
 */
var FLOAT_MIN = 1.175494351E-38


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
   * - If n is a number, it creates a new Int16Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int16Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  Int16Array(n) {
    self._ptr = _array.Int16Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns int
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT16_MIN or value > INT16_MAX
      raise Exception('value out of int16 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.int16_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param number value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value) and !is_int(value)
      raise Exception('Int16Array stores integer values')
    
    return _array.int16_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param Int16Array array
   */
  extend(array) {
    if !instance_of(array, Int16Array)
      raise Exception('instance of Int16Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int16_reverse(self._ptr)
  }

  /**
   * Returns a new Int16Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns Int16Array
   */
  clone() {
    return Int16Array(_array.int16_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.int16_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 2)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.int16_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw int16 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.int16___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
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
   * - If n is a number, it creates a new Int32Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int32Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  Int32Array(n) {
    self._ptr = _array.Int32Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 4
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns int
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns int
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT32_MIN or value > INT32_MAX
      raise Exception('value out of int32 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.int32_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param int value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value) and !is_int(value)
      raise Exception('Int32Array stores integer values')
    
    return _array.int32_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param Int32Array array
   */
  extend(array) {
    if !instance_of(array, Int32Array)
      raise Exception('instance of Int32Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int32_reverse(self._ptr)
  }

  /**
   * Returns a new Int32Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns Int32Array
   */
  clone() {
    return Int32Array(_array.int32_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.int32_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 4)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.int32_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw int32 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.int32___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
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
   * - If n is a number, it creates a new Int64Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int64Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  Int64Array(n) {
    self._ptr = _array.Int64Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 8
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT64_MIN or value > INT64_MAX
      raise Exception('value out of int64 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.int64_get(self._ptr, index)
  }

  /**
   * set(index: number, value: number)
   * 
   * Sets the value at the given index.
   * 
   * @param number index
   * @param number value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value)
      raise Exception('Int64Array stores numeric values')
    
    return _array.int64_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param Int64Array array
   */
  extend(array) {
    if !instance_of(array, Int64Array)
      raise Exception('instance of Int64Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.int64_reverse(self._ptr)
  }

  /**
   * Returns a new Int64Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns Int64Array
   */
  clone() {
    return Int64Array(_array.int64_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.int64_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 8)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.int64_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw int64 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.int64___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
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
   * - If n is a number, it creates a new UInt16Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt16Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  UInt16Array(n) {
    self._ptr = _array.UInt16Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 2
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT16_MIN or value > INT16_MAX
      raise Exception('value out of uint16 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.uint16_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param number value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value) and !is_int(value)
      raise Exception('UInt16Array stores integer values')
    
    return _array.uint16_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param UInt16Array array
   */
  extend(array) {
    if !instance_of(array, UInt16Array)
      raise Exception('instance of UInt16Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint16_reverse(self._ptr)
  }

  /**
   * Returns a new UInt16Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns UInt16Array
   */
  clone() {
    return UInt16Array(_array.uint16_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.uint16_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 2)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.uint16_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw uint16 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.uint16___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
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
   * - If n is a number, it creates a new UInt32Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt32Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  UInt32Array(n) {
    self._ptr = _array.UInt32Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 4
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT32_MIN or value > INT32_MAX
      raise Exception('value out of uint32 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.uint32_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param number value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value) and !is_int(value)
      raise Exception('UInt32Array stores integer values')
    
    return _array.uint32_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param UInt32Array array
   */
  extend(array) {
    if !instance_of(array, UInt32Array)
      raise Exception('instance of UInt32Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint32_reverse(self._ptr)
  }

  /**
   * Returns a new UInt32Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns UInt32Array
   */
  clone() {
    return UInt32Array(_array.uint32_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.uint32_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 4)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.uint32_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw uint32 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.uint32___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
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
   * - If n is a number, it creates a new UInt64Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new UInt64Array with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  UInt64Array(n) {
    self._ptr = _array.UInt64Array(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 8
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < INT64_MIN or value > INT64_MAX
      raise Exception('value out of uint64 range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.uint64_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param number value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value)
      raise Exception('UInt64Array stores numeric values')
    
    return _array.uint64_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param UInt64Array array
   */
  extend(array) {
    if !instance_of(array, UInt64Array)
      raise Exception('instance of UInt64Array expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint64_reverse(self._ptr)
  }

  /**
   * Returns a new UInt64Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns UInt64Array
   */
  clone() {
    return UInt64Array(_array.uint64_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.uint64_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 8)
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _array.uint64_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw uint64 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.uint64___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}



/**
 * class FloatArray represents an array of 32-bit floating point numbers 
 * corresponding to the C float data type in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class FloatArray {

  /**
   * - If n is a number, it creates a new FloatArray that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new FloatArray with its elements set to 
   * the values in the list.
   * 
   * @param number|list n
   * @constructor
   */
  FloatArray(n) {
    self._ptr = _array.FloatArray(n)
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return _array.length(self._ptr)
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self.length() * 4
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  first() {
    return _array.first(self._ptr)
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number
   */
  last() {
    return _array.last(self._ptr)
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise Exception('integer expected')
    if value < FLOAT_MIN or value > FLOAT_MAX
      raise Exception('value out of float range')

    _array.append(self._ptr, value)
  }

  /**
   * Returns the number at the specified index in the array. If index is 
   * outside the boundary of the array indexes (0..(array.length() - 1)), 
   * an exception is thrown.
   * 
   * @param number index
   * @returns number
   */
  get(index) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    
    return _array.float_get(self._ptr, index)
  }

  /**
   * Sets the value at the given index.
   * 
   * @param number index
   * @param int value
   * @returns number
   */
  set(index, value) {
    if !is_number(index)
      raise Exception('Arrays are numerically indexed')
    if !is_number(value)
      raise Exception('FloatArray stores numeric values')
    
    return _array.float_set(self._ptr, index, value)
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param FloatArray array
   */
  extend(array) {
    if !instance_of(array, FloatArray)
      raise Exception('instance of FloatArray expected')
    _array.extend(self._ptr, array.get_pointer())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   */
  reverse() {
    _array.uint64_reverse(self._ptr)
  }

  /**
   * Returns a new FloatArray containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns FloatArray
   */
  clone() {
    return FloatArray(_array.float_clone(self._ptr))
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number
   */
  pop() {
    return _array.float_pop(self._ptr)
  }

  /**
   * Returns the array as a bytes object
   * 
   * @returns bytes
   */
  to_bytes() {
    return _array.to_bytes(self._ptr, 8)
  }

  /**
   * Returns the elements of the array as a list of numbers
   * 
   * @returns list
   */
  to_list() {
    return _array.float_to_list(self._ptr)
  }

  /**
   * Returns a string representation of the array
   * 
   * @returns string
   */
  to_string() {
    return _array.to_string(self._ptr)
  }

  /**
   * Returns the raw uint64 array pointer.
   * 
   * @returns ptr
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
      raise Exception('Arrays are numerically indexed')
    return _array.uint64___iter__(self._ptr, n)
  }

  @itern(n) {
    if !is_number(n)
      raise Exception('Arrays are numerically indexed')
    return _array.itern(self._ptr, n)
  }
}
