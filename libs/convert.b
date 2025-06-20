/**
 * @module convert
 *
 * This module provides extra data conversion features between non-standard 
 * object types as well as different number bases.
 * 
 * @copyright 2021, Richard Ore and Blade contributors
 */


var _hex_table = '0123456789abcdef'
var _reverse_hex_table = ['0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F']

/**
 * Converts hexadecimal string of any length to bytes.
 * 
 * @param string str
 * @returns bytes
 */
def hex_to_bytes(str) {
  if !is_string(str)
    raise TypeError('string expected, ${typeof(str)} given')

  # str = ''.join(str.to_list().reverse())

  var length = str.length()
  var b = [0] * (length / 2)

  iter var i = 0; i < length - 1; i += 2 {
    b[i / 2] = to_number('0x${str[i,i+2]}')
  }
  if length % 2 != 0 b.append(_reverse_hex_table.index_of(str[-1].upper()))

  return bytes(b.reverse())
}

/**
 * Converts binary data (byes) of any length to hexadecimal string 
 * representation.
 * 
 * @param bytes data
 * @returns string
 */
def bytes_to_hex(data) {
  if !is_bytes(data)
    raise TypeError('bytes expected, ${typeof(data)} given')

  var hex_string = ''

  for b in data {
    hex_string += hex(b).lpad(2, '0')
  }

  return hex_string
}

/**
 * Converts the given decimal based number to an hexadecimal string.
 * 
 * @param number n
 * @returns string
 */
def decimal_to_hex(n) {
  if !is_number(n)
    raise TypeError('number expected, ${typeof(n)} given')

  var hex_list = []
  while n > 0 {
    hex_list.append(_hex_table[n % 16])
    n = int(n / 16)
  }

  return ''.join(hex_list.reverse())
}

/**
 * Converts the given hexadecimal string to a decimal base 10 number.
 * 
 * @note string must either contain the plain hexadecimal string or be in the format 0x[hex string].
 * @param string str
 * @returns number
 */
def hex_to_decimal(str) {
  if !is_string(str)
    raise TypeError('string expected, ${typeof(str)} given')

  if str.starts_with('0x')
    str = str[2,]

  var l = str.length(), hex = 0
  str = str.upper()

  iter var i = 0; i < l; i++ {
    hex += _reverse_hex_table.index_of(str[i]) * (16 ** (l - i - 1))
  }

  return hex
}

/**
 * Converts a unicode character to it's equivalent hexadecimal string.
 * 
 * @param char chr
 * @returns string
 */
def unicode_to_hex(chr) {
  if !is_string(chr) or chr.length() > 1 {
    raise ValueError('char expected, ${typeof(chr)} given')
  }

  return decimal_to_hex(ord(chr))
}

/**
 * Converts a bytes (binary data) to a decimal number.
 *
 * @param bytes bytes
 * @returns number
 */
def bytes_to_decimal(bytes) {
  if !is_bytes(bytes)
    raise TypeError('bytes expected, ${typeof(bytes)} given')

  var decimal = 0
  for byte in bytes {
    decimal <<= 8
    decimal |= byte
  }

  return decimal
}
