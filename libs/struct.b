/**
 * @module struct
 * 
 * This module provides functions for converting between Blade values and C
 * structs and vice-versa in the binary format.
 * 
 * @copyright 2022, Ore Richard Muyiwa and Blade contributors
 */

import _struct

/**
 * Packs the given arguments into a bytes object according to the specified format. 
 * This function behaves the same as the pack function from Perl and PHP (more similar 
 * to the PHP version) and uses the same formatting rules as the PHP version.
 * 
 * -  The `format` string consists of format codes followed by an optional repeater argument. 
 *    The repeater argument can be either an integer value or `*` for repeating to the end of the 
 *    input data. For `a`, `A`, `h`, `H` the repeat count specifies how many characters of one data 
 *    argument are taken, for `@` it is the absolute position where to put the next data, for 
 *    everything else the repeat count specifies how many data arguments are consumed and packed 
 *    into the resulting binary string.
 * 
 * - The accepted characters in the format string are as defined below:
 * 
 *  Code  | Description 
 *  ------|-------------
 *  a     | NUL-padded string 
 *  A     | SPACE-padded string 
 *  h     | Hex string, low nibble first 
 *  H     | Hex string, high nibble first 
 *  c     | signed char 
 *  C     | unsigned char 
 *  s     | signed short (always 16 bit, machine byte order) 
 *  S     | unsigned short (always 16 bit, machine byte order) 
 *  n     | unsigned short (always 16 bit, big endian byte order) 
 *  v     | unsigned short (always 16 bit, little endian byte order) 
 *  i     | signed integer (machine dependent size and byte order) 
 *  I     | unsigned integer (machine dependent size and byte order) 
 *  l     | signed long (always 32 bit, machine byte order) 
 *  L     | unsigned long (always 32 bit, machine byte order) 
 *  N     | unsigned long (always 32 bit, big endian byte order) 
 *  V     | unsigned long (always 32 bit, little endian byte order) 
 *  q     | signed long long (always 64 bit, machine byte order) 
 *  Q     | unsigned long long (always 64 bit, machine byte order) 
 *  J     | unsigned long long (always 64 bit, big endian byte order) 
 *  P     | unsigned long long (always 64 bit, little endian byte order) 
 *  f     | float (machine dependent size and representation) 
 *  g     | float (machine dependent size, little endian byte order) 
 *  G     | float (machine dependent size, big endian byte order) 
 *  d     | double (machine dependent size and representation) 
 *  e     | double (machine dependent size, little endian byte order) 
 *  E     | double (machine dependent size, big endian byte order) 
 *  x     | NUL byte 
 *  X     | Back up one byte 
 *  Z     | NUL-padded string 
 *  @     | NUL-fill to absolute position 
 * 
 * @param string format
 * @param any... \__args__
 * @returns bytes
 */
def pack(format, ...) {
  if !is_string(format)
    raise Exception('format string expected')

  var items = []
  for arg in __args__ {
    if is_list(arg) items.extend(arg)
    else if is_bytes(arg) items.extend(arg.to_list())
    else items.append(arg)
  }
  return _struct.pack(format, items)
}

/**
 * Unpacks from bytes or a string into a dictionary based on the given format.
 * 
 * -  You may have to name the different format codes and separate them by a slash `/` 
 *    to return a string indexed dictionary for easy reference of the destructed parts. 
 *    If a repeater argument is present, then each of the dictionary keys will have a 
 *    sequence number behind the given name.
 * 
 * -  See `pack()` for format options
 * -  `offset` is the index of the bytes/string to begin unpacking from
 * 
 * > **Important**
 * > If you do not name an element, numeric indices starting from 1 are used. Be aware 
 * > that if you have more than one unnamed element, some data is overwritten because the 
 * > numbering restarts from 1 for each element.
 * 
 * @param string format
 * @param bytes|string data
 * @param number? offset: Default value is `0`
 * @returns any
 */
def unpack(format, data, offset) {
  if !is_string(format)
    raise Exception('format string expected in argument 1')
  if !is_string(data) and !is_bytes(data)
    raise Exception('data must be string or bytes')
  if offset != nil and !is_number(offset)
    raise Exception('offset must be number if set')

  if is_string(data) data = data.to_bytes()
  if !offset offset = 0
  
  return _struct.unpack(format, data, offset)
}

/**
 * Same as `pack()` except that instead of accepting arbitrary values after 
 * format, it expects the values to be in a list.
 * 
 * @param string format
 * @param list args
 * @returns bytes
 */
def pack_from(format, args) {
  if !is_string(format)
    raise Exception('format string expected in argument 1')
  if !is_list(args)
    raise Exception('list expected in argument 2 (args)')

  return _struct.pack(format, args)
}
