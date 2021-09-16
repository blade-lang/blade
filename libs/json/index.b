#
# @module json
#
# provides APIs for encoding and decoding json data
# @copyright 2021, Ore Richard Muyiwa
#

import .encoder { * }
import _json { _decode }


/**
 * encode(value: any [, compact: boolean = false [, max_depth: number = 1024]])
 * 
 * JSON encodes the given value with a recursive depth up to
 * @max_depth. 
 * 
 * @param compact indicates whether the resulting json string will be tightly
 * packed. i.e. spaces will be trimed from objects and arrays.
 * @param max_depth is the maximum recursive depth for encoding, default = 1024.
 * @return string
 */
def encode(value, compact, max_depth) {
  return Encoder(compact, max_depth).encode(value)
}

/**
 * decode(value: string [, allow_comments: boolean = true])
 * 
 * decodes the input JSON string into Blade objects
 * 
 * @param value is the string to decode
 * @param allow_comments can be set to enable/disable C-style comments in json
 * [default = true]
 * @return object
 */
def decode(value, allow_comments) {
  if allow_comments == nil allow_comments = true
  return _decode(value, allow_comments)
}

/**
 * parse(path: string)
 * 
 * parses a file containing json data.
 * @return object
 */
def parse(path) {
  if !is_string(path)
    die Exception('file path expected, ${typeof(path)} given')

  var f = file(path)
  if !f.exists()
    die Exception('could not open file ${path}')

  return decode(f.read())
}

