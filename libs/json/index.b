#
# @module json
#
# provides APIs for encoding and decoding json data
# @copyright 2021, Ore Richard Muyiwa
#

import .encoder { * }
import .decoder { * }


/**
 * encode(value: any [, compact: boolean = false [, max_depth: number = 1024]])
 * 
 * JSON encodes the given value with a recursive depth up to
 * @max_depth. 
 * @note if compact is true, the resulting json string will be tightly
 * packed. i.e. spaces will be trimed from objects and arrays.
 * @note if max_depth is not given, max_depth = 1024.
 */
def encode(value, compact, max_depth) {
  return Encoder(compact, max_depth).encode(value)
}