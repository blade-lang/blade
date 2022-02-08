#
# @module convert
#
# This module provides extra data conversion features between non-standard 
# object types as well as different number bases.
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#


var _hex_table = '0123456789abcdef'
var _reverse_hex_table = ['0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F']

/**
 * hex_to_bytes(str)
 *
 * converts hexadecimal string of any length to bytes
 * @return bytes
 */
def hex_to_bytes(str) {
  if !is_string(str)
    die Exception('string expected, ${typeof(str)} given')

  var length = str.length()
  var b = bytes(length / 2)

  iter var i = 0; i < length - 1; i += 2 {
    b[i / 2] = to_number('0x${str[i,i+2]}')
  }

  return b
}

/**
 * decimal_to_hex(n: number)
 * 
 * converts the given decimal based number to an hexadeccimal string
 * @return string
 */
def decimal_to_hex(n) {
  if !is_number(n)
    die Exception('number expected, ${typeof(n)} given')

  var result = []
  while n > 0 {
    result.append(_hex_table[n % 16])
    n = int(n / 16)
  }
  return ''.join(result.reverse())
}

/**
 * hex_to_decimal(str: string)
 * 
 * converts the given hexadecimal string to a decimal base 10 number
 * @note string must either contain the plain hexadecimal string or be in the format 0x[hex string].
 * @return number
 */
def hex_to_decimal(str) {
  if !is_string(str)
    die Exception('string expected, ${typeof(str)} given')

  if str.starts_with('0x')
    str = str[2,]

  var l = str.length(), result = 0
  str = str.upper()

  iter var i = 0; i < l; i++ {
    result += _reverse_hex_table.index_of(str[i]) * (16 ** (l - i - 1))
  }
  return result
}

/**
 * unicode_to_hex(chr: char)
 * 
 * converts a unicode character to it's equivalent hexadecimal string.
 * @return string
 */
def unicode_to_hex(chr) {
  if !is_string(chr) or chr.length() > 1
    die Exception('char expected, ${typeof(chr)} given')

  return decimal_to_hex(ord(chr))
}
