import _struct


/**
 * Maximum value that "should" exist in a list passed to Int64Array.
 * @type number
 */
var INT64_MAX = 9223372036854775807

/**
 * Minimum value that "should" exist in a list passed to Int64Array.
 * @type number
 */
var INT64_MIN = -INT64_MAX - 1


/**
 * class Int64Array represents an array of twos-complement 16-bit signed 
 * integers in the platform byte order.
 * 
 * @printable
 * @iterable
 * @serializable
 */
class Int64Array {

  var _data
  var _bit_size = 8
  var _data_type = 'q'

  /**
   * - If n is a number, it creates a new Int64Array that can hold up to n 
   * number of elements, but with all the elements set to 0. 
   * - If n is a list, it creates a new Int64Array with its elements set to 
   * the values in the list.
   * 
   * @param {number|list} n
   * @constructor
   */
  Int64Array(n) {
    if is_number(n) {
      self._data = bytes(n * self._bit_size)
    } else if is_list(n) {
      # validate
      for item in n {
        if !is_number(item) {
          raise ValueError('invalid Int64Array value')
        }
      }

      self._data = _struct.pack('${self._data_type}${n.length()}', n)
    } else {
      raise TypeError('number or list expected, ${typeof(n)} given')
    }
  }

  /**
   * Returns the number of items in the array. 
   * 
   * @returns number
   */
  length() {
    return self._data.length() // self._bit_size
  }

  /**
   * Returns the length of the array if it were to be converted to bytes.
   * 
   * @returns number
   */
  bytes_length() {
    return self._data.length()
  }

  /**
   * Returns the first item in the array or nil if the array is empty.
   * 
   * @returns number?
   */
  first() {
    if self.length() > 0 {
      return _struct.unpack(
        self._data_type, 
        self._data[,self._bit_size], 
        0
      ).values().first()
    }

    return nil
  }

  /**
   * Returns the last item in the array or nil if the array is empty.
   * 
   * @returns number?
   */
  last() {
    if self.length() > 0 {
      return _struct.unpack(
        self._data_type, 
        self._data[self._data.length() - self._bit_size,], 
        0
      ).values().first()
    }

    return nil
  }

  /**
   * Adds the given _value_ to the end of the array.
   * 
   * @param int value
   */
  append(value) {
    if !is_number(value) or !is_int(value)
      raise TypeError('integer expected')
    if value < INT64_MIN or value > INT64_MAX
      raise ValueError('value out of int64 range')

    var as_bytes = _struct.pack(self._data_type, [value])
    self._data.extend(as_bytes)
    as_bytes.dispose()
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
      raise ArgumentError('Arrays are numerically indexed')

    if self.length() > index {
      var start = index * self._bit_size
      return _struct.unpack(
        self._data_type, 
        self._data[start, start + self._bit_size], 
        0
      ).values().first()
    }

    return nil
  }

  _do_set(start, as_bytes) {
    self._data[start] = as_bytes[0]
    self._data[start + 1] = as_bytes[1]
    self._data[start + 2] = as_bytes[2]
    self._data[start + 3] = as_bytes[3]
    self._data[start + 4] = as_bytes[4]
    self._data[start + 5] = as_bytes[5]
    self._data[start + 6] = as_bytes[6]
    self._data[start + 7] = as_bytes[7]
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
      raise ArgumentError('Arrays are numerically indexed')
    if !is_number(value) and !is_int(value)
      raise ArgumentError('Int64Array stores integer values')

    var as_bytes = _struct.pack(self._data_type, [value])
    var start = index * self._bit_size
    
    if self.length() <= index {
      var diff = index - self.length() + 1 # +1 for the new data

      var extension = bytes(diff * self._bit_size)
      self._data.extend(extension)
      extension.dispose()
    }

    self._do_set(start, as_bytes)
    as_bytes.dispose()

    return value
  }

  /**
   * Updates the content of the current array by appending all the contents 
   * of _array_ to the end of the array in exact order.
   * 
   * @param Int64Array array
   */
  extend(array) {
    if !instance_of(array, Int64Array)
      raise TypeError('instance of Int64Array expected')
    self._data.extend(array.to_bytes())
  }

  /**
   * Returns a new array containing the elements in the original array 
   * in reverse order.
   * 
   * @returns [[array.Int64Array]]
   */
  reverse() {
    var data = self.to_list().reverse()
    return Int64Array(data)
  }

  /**
   * Returns a new Int64Array containing all items from the current array. 
   * The new array is a shallow copy of the original array.
   * 
   * @returns Int64Array
   */
  clone() {
    return Int64Array(self.to_list())
  }

  /**
   * Removes the last element in the array and returns the value of that item.
   * 
   * @returns number?
   */
  pop() {
    var last = self.last()
    if last != nil {
      self._data = self._data[,-self._bit_size]
    }

    return last
  }

  /**
   * Returns the array as a bytes object.
   * 
   * @returns bytes
   */
  to_bytes() {
    return self._data.clone()
  }

  /**
   * Returns the elements of the array as a list of numbers.
   * 
   * @returns list
   */
  to_list() {
    return _struct.unpack(
      '${self._data_type}${self.length()}', 
      self._data, 
      0
    ).values()
  }

  /**
   * Returns a string representation of the array.
   * 
   * @returns string
   */
  to_string() {
    return self._data.to_string()
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
      raise ArgumentError('Arrays are numerically indexed')
    return self.get(n)
  }

  @itern(n) {
    if index == nil return 0
    if !is_number(index)
      raise ArgumentError('Arrays are numerically indexed')
    if index < self.length() - 1 return index + 1
    return nil
  }
}
